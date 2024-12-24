#include "AnalyzeContext.h"

#include "CLucene/util/CLStreams.h"

CL_NS_USE2(analysis, ik)

AnalyzeContext::AnalyzeContext(std::shared_ptr<Configuration> configuration)
        : buffer_offset_(0),
          cursor_(0),
          available_(0),
          last_useless_char_num_(0),
          config_(configuration) {
    segment_buff_.resize(BUFF_SIZE);
}

AnalyzeContext::~AnalyzeContext() = default;

void AnalyzeContext::reset() {
    buffer_locker_ = 0;
    org_lexemes_.clear();
    available_ = 0;
    buffer_offset_ = 0;
    cursor_ = 0;
    last_useless_char_num_ = 0;
    typed_runes_.clear();
    path_map_.clear();
    results_.clear();
}

size_t AnalyzeContext::fillBuffer(lucene::util::Reader* reader) {
    int32_t readCount = 0;
    if (buffer_offset_ == 0) {
        readCount = reader->readCopy(segment_buff_.data(), 0, BUFF_SIZE);

        readCount = CharacterUtil::adjustToCompleteChar(segment_buff_.data(), readCount);

        CharacterUtil::processString(segment_buff_.c_str(), readCount, typed_runes_,
                                     config_->isEnableLowercase());
    } else {
        size_t offset = available_ - typed_runes_[cursor_].getNextBytePosition();
        if (offset > 0) {
            memmove(segment_buff_.data(),
                    segment_buff_.data() + typed_runes_[cursor_].getNextBytePosition(), offset);
            readCount = std::max(
                    0, reader->readCopy(segment_buff_.data() + offset, 0, BUFF_SIZE - offset));
            readCount =
                    CharacterUtil::adjustToCompleteChar(segment_buff_.data() + offset, readCount) +
                    offset;
        } else {
            readCount = std::max(0, reader->readCopy(segment_buff_.data(), 0, BUFF_SIZE));
            readCount = CharacterUtil::adjustToCompleteChar(segment_buff_.data(), readCount);
        }

        CharacterUtil::TypedRuneArray().swap(typed_runes_);
        CharacterUtil::processString(segment_buff_.c_str(), readCount, typed_runes_,
                                     config_->isEnableLowercase());
    }

    available_ = readCount;
    cursor_ = 0;
    return readCount;
}

void AnalyzeContext::addLexeme(Lexeme lexeme) {
    org_lexemes_.addLexeme(std::move(lexeme));
}

void AnalyzeContext::addLexemePath(std::unique_ptr<LexemePath> path) {
    if (path) {
        auto begin = path->getPathBegin();
        path_map_.emplace(begin, std::move(path));
    }
}

void AnalyzeContext::compound(Lexeme& lexeme) {
    if (!config_->isUseSmart()) {
        return;
    }

    if (!results_.empty()) {
        if (Lexeme::Type::Arabic == lexeme.getType()) {
            auto& nextLexeme = results_.front();
            bool appendOk = false;
            if (Lexeme::Type::CNum == nextLexeme.getType()) {
                appendOk = lexeme.append(nextLexeme, Lexeme::Type::CNum);
            } else if (Lexeme::Type::Count == nextLexeme.getType()) {
                appendOk = lexeme.append(nextLexeme, Lexeme::Type::CQuan);
            }
            if (appendOk) {
                results_.pop_front();
            }
        }

        if (Lexeme::Type::CNum == lexeme.getType() && !results_.empty()) {
            auto nextLexeme = results_.front();
            bool appendOk = false;
            if (Lexeme::Type::Count == nextLexeme.getType()) {
                appendOk = lexeme.append(nextLexeme, Lexeme::Type::CQuan);
            }
            if (appendOk) {
                results_.pop_front();
            }
        }
    }
    return;
}

bool AnalyzeContext::moveCursor() {
    if (cursor_ < typed_runes_.size() - 1) {
        cursor_++;
        return true;
    }
    return false;
}

void AnalyzeContext::initCursor() {
    cursor_ = 0;
    typed_runes_[cursor_].regularize(config_->isEnableLowercase());
}

bool AnalyzeContext::isBufferConsumed() const {
    return cursor_ == typed_runes_.size() - 1;
}

bool AnalyzeContext::needRefillBuffer() const {
    return !isBufferLocked() && cursor_ < typed_runes_.size() - 1 &&
           cursor_ > typed_runes_.size() - BUFF_EXHAUST_CRITICAL;
}

void AnalyzeContext::markBufferOffset() {
    buffer_offset_ += typed_runes_[cursor_].offset;
}

void AnalyzeContext::lockBuffer(const std::string& segmenterName) {
    if (segmenterName == "CJK_SEGMENTER")
        buffer_locker_ |= CJK_SEGMENTER_FLAG;
    else if (segmenterName == "CN_QUANTIFIER")
        buffer_locker_ |= CN_QUANTIFIER_FLAG;
    else if (segmenterName == "LETTER_SEGMENTER")
        buffer_locker_ |= LETTER_SEGMENTER_FLAG;
}

void AnalyzeContext::unlockBuffer(const std::string& segmenterName) {
    if (segmenterName == "CJK_SEGMENTER")
        buffer_locker_ &= ~CJK_SEGMENTER_FLAG;
    else if (segmenterName == "CN_QUANTIFIER")
        buffer_locker_ &= ~CN_QUANTIFIER_FLAG;
    else if (segmenterName == "LETTER_SEGMENTER")
        buffer_locker_ &= ~LETTER_SEGMENTER_FLAG;
}

bool AnalyzeContext::isBufferLocked() const {
    return buffer_locker_ != 0;
}

std::optional<Lexeme> AnalyzeContext::getNextLexeme() {
    if (results_.empty()) {
        return std::nullopt;
    }

    auto result = std::move(results_.front());
    results_.pop_front();

    while (true) {
        compound(result);
        if (Dictionary::getSingleton()->isStopWord(typed_runes_, result.getCharBegin(),
                                                   result.getCharLength())) {
            if (results_.empty()) {
                return std::nullopt;
            }
            result = std::move(results_.front());
            results_.pop_front();
        } else {
            result.setText(std::string(segment_buff_.data() + result.getByteBegin(),
                                       result.getByteLength()));
            break;
        }
    }
    return result;
}

void AnalyzeContext::outputToResult() {
    for (size_t index = 0; index <= cursor_;) {
        if (typed_runes_[index].char_type == CharacterUtil::CHAR_USELESS) {
            index++;
            last_useless_char_num_++;
            continue;
        }
        last_useless_char_num_ = 0;
        auto byte_pos = typed_runes_[index].getBytePosition();
        auto pathIter = path_map_.find(byte_pos);
        if (pathIter != path_map_.end()) {
//            auto& path = pathIter->second;
            auto& path = pathIter->value;  // 注意这里改成了 value

            while (auto lexeme = path->pollFirst()) {
                results_.push_back(std::move(*lexeme));
                index = lexeme->getCharEnd() + 1;
                auto next_lexeme = path->peekFirst();
                if (next_lexeme) {
                    for (; index < next_lexeme->getCharBegin(); index++) {
                        outputSingleCJK(index);
                    }
                }
            }
        } else {
            outputSingleCJK(index);
            index++;
        }
    }
    IKFlatMap<size_t, std::unique_ptr<LexemePath>>().swap(path_map_);
}

void AnalyzeContext::outputSingleCJK(size_t index) {
    if (typed_runes_[index].char_type == CharacterUtil::CHAR_CHINESE ||
        typed_runes_[index].char_type == CharacterUtil::CHAR_OTHER_CJK) {
        results_.emplace_back(buffer_offset_, typed_runes_[index].offset, typed_runes_[index].len,
                              typed_runes_[index].char_type == CharacterUtil::CHAR_CHINESE
                                      ? Lexeme::Type::CNChar
                                      : Lexeme::Type::OtherCJK,
                              index, index);
    }
}
