#include "CJKSegmenter.h"
CL_NS_USE2(analysis, ik)

const std::string CJKSegmenter::SEGMENTER_NAME = "CJK_SEGMENTER";

CJKSegmenter::CJKSegmenter() = default;

void CJKSegmenter::analyze(std::shared_ptr<AnalyzeContext> context) {
    if (context == nullptr) return;

    if (context->getCurrentCharType() != CharacterUtil::CHAR_USELESS) {
        if (!tmp_hits_.empty()) {
            auto it = tmp_hits_.begin();
            while (it != tmp_hits_.end()) {
                Hit& hit = *it;
                Dictionary::getSingleton()->matchWithHit(context->getTypedRuneArray(),
                                                         context->getCursor(), hit);

                if (hit.isMatch()) {
                    Lexeme newLexeme(context->getBufferOffset(), hit.getByteBegin(),
                                     hit.getByteEnd() - hit.getByteBegin(), Lexeme::Type::CNWord,
                                     hit.getCharBegin(), hit.getCharEnd());
                    context->addLexeme(std::move(newLexeme));

                    if (!hit.isPrefix()) {
                        it = tmp_hits_.erase(it);
                    } else {
                        ++it;
                    }
                } else if (hit.isUnmatch()) {
                    it = tmp_hits_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        auto singleCharHit = Dictionary::getSingleton()->matchInMainDict(
                context->getTypedRuneArray(), context->getCursor(), 1);

        if (singleCharHit.isMatch()) {
            Lexeme newLexeme(context->getBufferOffset(), context->getCurrentCharOffset(),
                             context->getCurrentCharLen(), Lexeme::Type::CNChar,
                             singleCharHit.getCharBegin(), singleCharHit.getCharEnd());
            context->addLexeme(std::move(newLexeme));
        }
        if (singleCharHit.isPrefix()) {
            tmp_hits_.push_back(singleCharHit);
        }
    } else {
        reset();
    }

    if (context->isBufferConsumed()) {
        reset();
    }

    if (tmp_hits_.empty()) {
        context->unlockBuffer(SEGMENTER_NAME);
    } else {
        context->lockBuffer(SEGMENTER_NAME);
    }
}

void CJKSegmenter::reset() {
    tmp_hits_.clear();
}