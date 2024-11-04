#include "IKTokenizer.h"

#include "CLucene/analysis/ik/core/IKSegmenter.h"
#include "CLucene/util/CLStreams.h"

CL_NS_DEF2(analysis, ik)
CL_NS_USE(analysis)
CL_NS_USE(util)

IKTokenizer::IKTokenizer(Reader* reader, std::shared_ptr<Configuration> cfg)
        : Tokenizer(reader), config_(std::move(cfg)) {
    reset(reader);
    Tokenizer::lowercase = false;
    Tokenizer::ownReader = false;
}

IKTokenizer::IKTokenizer(Reader* reader, bool isSmart, bool lowercase, bool ownReader) {
    reset(reader);
    config_->setUseSmart(isSmart);
    config_->setEnableLowercase(lowercase);
    Tokenizer::lowercase = lowercase;
    Tokenizer::ownReader = ownReader;
}

Token* IKTokenizer::next(Token* token) {
    if (buffer_index_ >= data_length_) {
        return nullptr;
    }

    std::string& token_text = tokens_text_[buffer_index_++];
    size_t size = std::min(token_text.size(), static_cast<size_t>(LUCENE_MAX_WORD_LEN));
    token->setNoCopy(token_text.data(), 0, size);
    return token;
}

void IKTokenizer::reset(Reader* reader) {
    this->input = reader;
    this->buffer_index_ = 0;
    this->data_length_= 0;
    this->tokens_text_.clear();

    buffer_.reserve(input->size());
    IKSegmenter segmenter(reader, config_);

    std::shared_ptr<Lexeme> lexeme = nullptr;
    while ((lexeme = segmenter.next()) != nullptr) {
        tokens_text_.emplace_back(lexeme->getText());
    }

    data_length_ = tokens_text_.size();
}

CL_NS_END2