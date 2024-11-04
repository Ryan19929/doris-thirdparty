#include "LetterSegmenter.h"
CL_NS_USE2(analysis, ik)

const std::string LetterSegmenter::SEGMENTER_NAME = "LETTER_SEGMENTER";

LetterSegmenter::LetterSegmenter()
        : letter_connectors_ {'#', '&', '+', '-', '.', '@', '_'}, num_connectors_ {',', '.'} {
    std::sort(std::begin(letter_connectors_), std::end(letter_connectors_));
    std::sort(std::begin(num_connectors_), std::end(num_connectors_));
}

void LetterSegmenter::analyze(std::shared_ptr<AnalyzeContext> context) {
    bool bufferLockFlag = false;
    bufferLockFlag = processEnglishLetter(context) || bufferLockFlag;
    bufferLockFlag = processArabicLetter(context) || bufferLockFlag;
    bufferLockFlag = processMixLetter(context) || bufferLockFlag;

    if (bufferLockFlag) {
        context->lockBuffer(SEGMENTER_NAME);
    } else {
        context->unlockBuffer(SEGMENTER_NAME);
    }
}

void LetterSegmenter::reset() {
    start_ = -1;
    end_ = -1;
    english_start_ = -1;
    english_end_ = -1;
    arabic_start_ = -1;
    arabic_end_ = -1;
}

bool LetterSegmenter::processEnglishLetter(shared_ptr<AnalyzeContext> context) {
    bool need_lock = false;

    const auto& typed_runes = context->getTypedRuneArray();
    if (english_start_ == -1) {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ENGLISH) {
            english_start_ = context->getCursor();
            english_end_ = english_start_;
        }
    } else {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ENGLISH) {
            english_end_ = context->getCursor();
        } else {
            auto newLexeme = std::make_shared<Lexeme>(
                    context->getBufferOffset(), typed_runes[english_start_].offset,
                    english_end_ - english_start_ + 1, Lexeme::Type::English, english_start_, english_end_);
            context->addLexeme(std::move(newLexeme));
            english_start_ = -1;
            english_end_ = -1;
        }
    }

    if (context->isBufferConsumed() && (english_start_ != -1 && english_end_ != -1)) {
        auto newLexeme = std::make_shared<Lexeme>(
                context->getBufferOffset(), typed_runes[english_start_].offset,
                english_end_ - english_start_ + 1, Lexeme::Type::English, english_start_, english_end_);
        context->addLexeme(std::move(newLexeme));
        english_start_ = -1;
        english_end_ = -1;
    }

    if (english_start_ == -1 && english_end_ == -1) {
        need_lock = false;
    } else {
        need_lock = true;
    }
    return need_lock;
}

bool LetterSegmenter::processArabicLetter(shared_ptr<AnalyzeContext> context) {
    bool need_lock = false;
    const auto& typed_runes = context->getTypedRuneArray();

    if (arabic_start_ == -1) {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ARABIC) {
            arabic_start_ = context->getCursor();
            arabic_end_ = arabic_start_;
        }
    } else {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ARABIC) {
            arabic_end_ = context->getCursor();
        } else if (context->getCurrentCharType() == CharacterUtil::CHAR_USELESS &&
                   isNumConnector(context->getCurrentChar())) {
        } else {
            auto newLexeme = std::make_shared<Lexeme>(
                    context->getBufferOffset(), typed_runes[arabic_start_].offset,
                    arabic_end_ - arabic_start_ + 1, Lexeme::Type::Arabic, arabic_start_, arabic_end_);
            context->addLexeme(std::move(newLexeme));
            arabic_start_ = -1;
            arabic_end_ = -1;
        }
    }

    if (context->isBufferConsumed() && (arabic_start_ != -1 && arabic_end_ != -1)) {
        auto newLexeme = std::make_shared<Lexeme>(
                context->getBufferOffset(), typed_runes[arabic_start_].offset,
                arabic_end_ - arabic_start_ + 1, Lexeme::Type::Arabic, arabic_start_, arabic_end_);
        context->addLexeme(std::move(newLexeme));
        arabic_start_ = -1;
        arabic_end_ = -1;
    }

    if (arabic_start_ == -1 && arabic_end_ == -1) {
        need_lock = false;
    } else {
        need_lock = true;
    }
    return need_lock;
}

bool LetterSegmenter::processMixLetter(shared_ptr<AnalyzeContext> context) {
    bool need_lock = false;
    const auto& typed_runes = context->getTypedRuneArray();

    if (start_ == -1) {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ARABIC ||
            context->getCurrentCharType() == CharacterUtil::CHAR_ENGLISH) {
            start_ = context->getCursor();
            end_ = start_;
        }
    } else {
        if (context->getCurrentCharType() == CharacterUtil::CHAR_ARABIC ||
            context->getCurrentCharType() == CharacterUtil::CHAR_ENGLISH) {
            end_ = context->getCursor();

        } else if (context->getCurrentCharType() == CharacterUtil::CHAR_USELESS &&
                   isLetterConnector(context->getCurrentChar())) {
            end_ = context->getCursor();
        } else {
            auto newLexeme = std::make_shared<Lexeme>(context->getBufferOffset(), typed_runes[start_].offset,
                                           end_ - start_ + 1, Lexeme::Type::Letter, start_, end_);
            context->addLexeme(std::move(newLexeme));
            start_ = -1;
            end_ = -1;
        }
    }

    if (context->isBufferConsumed() && (start_ != -1 && end_ != -1)) {
        auto newLexeme = std::make_shared<Lexeme>(context->getBufferOffset(), typed_runes[start_].offset,
                                       end_ - start_ + 1, Lexeme::Type::Letter, start_, end_);
        context->addLexeme(std::move(newLexeme));
        start_ = -1;
        end_ = -1;
    }

    need_lock = (start_ != -1 && end_ != -1);
    return need_lock;
}

bool LetterSegmenter::isLetterConnector(char input) {
    return std::binary_search(std::begin(letter_connectors_), std::end(letter_connectors_), input);
}

bool LetterSegmenter::isNumConnector(char input) {
    return std::binary_search(std::begin(num_connectors_), std::end(num_connectors_), input);
}