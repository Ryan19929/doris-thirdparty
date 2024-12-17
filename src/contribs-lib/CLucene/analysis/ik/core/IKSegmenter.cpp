#include "IKSegmenter.h"
CL_NS_USE2(analysis, ik)

IKSegmenter::IKSegmenter(lucene::util::Reader* input, std::shared_ptr<Configuration> config)
        : input_(input), config_(config) {
    context_ = std::make_shared<AnalyzeContext>(config);
    init();
}

void IKSegmenter::init() {
    segmenters_ = loadSegmenters();
    arbitrator_ = IKArbitrator();
}

std::vector<std::unique_ptr<ISegmenter>> IKSegmenter::loadSegmenters() {
    std::vector<std::unique_ptr<ISegmenter>> segmenters;
    segmenters.push_back(std::make_unique<LetterSegmenter>());
    segmenters.push_back(std::make_unique<CN_QuantifierSegmenter>());
    segmenters.push_back(std::make_unique<CJKSegmenter>());
    return segmenters;
}

std::optional<Lexeme> IKSegmenter::next() {
    std::optional<Lexeme> lexeme = std::nullopt;
    while (!(lexeme = context_->getNextLexeme())) {
        int available = context_->fillBuffer(input_);
        if (available <= 0) {
            context_.reset();
            return std::nullopt;
        } else {
            context_->initCursor();
            do {
                for (const auto& segmenter : segmenters_) {
                    segmenter->analyze(context_);
                }
                if (context_->needRefillBuffer()) {
                    break;
                }
            } while (context_->moveCursor());
            for (const auto& segmenter : segmenters_) {
                segmenter->reset();
            }
        }
        arbitrator_.process(context_, config_->isUseSmart());
        context_->outputToResult();
        context_->markBufferOffset();
    }
    return lexeme;
}

void IKSegmenter::reset(lucene::util::Reader* newInput) {
    input_ = newInput;
    context_.reset();
    for (const auto& segmenter : segmenters_) {
        segmenter->reset();
    }
}

int IKSegmenter::getLastUselessCharNum() {
    return context_->getLastUselessCharNum();
}