#ifndef CLUCENE_ANALYZECONTEXT_H
#define CLUCENE_ANALYZECONTEXT_H

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include "CLucene/_ApiHeader.h"
#include "CLucene/analysis/ik/cfg/Configuration.h"
#include "CLucene/analysis/ik/dic/Dictionary.h"
#include "CLucene/analysis/ik/util/IKContainer.h"
#include "CLucene/util/CLStreams.h"
#include "CharacterUtil.h"
#include "LexemePath.h"
#include "QuickSortSet.h"

CL_NS_DEF2(analysis, ik)

class CLUCENE_EXPORT AnalyzeContext {
private:
    static const size_t BUFF_SIZE = 4096;
    static const size_t BUFF_EXHAUST_CRITICAL = 100;

    static constexpr uint8_t CJK_SEGMENTER_FLAG = 0x01;    // 0001
    static constexpr uint8_t CN_QUANTIFIER_FLAG = 0x02;    // 0010
    static constexpr uint8_t LETTER_SEGMENTER_FLAG = 0x04; // 0100

    // String buffer
    std::string segment_buff_;
    // Character information array
    CharacterUtil::TypedRuneArray typed_runes_;
    // Total length of bytes analyzed
    size_t buffer_offset_;
    // Current character position pointer
    size_t cursor_;
    // Length of available bytes in the last read
    size_t available_;
    // Number of non-CJK characters at the end
    size_t last_useless_char_num_;

    // Sub-tokenizer lock
    uint8_t buffer_locker_ {0};
    //std::set<std::string> buffer_locker_;
    // Original tokenization result set
    QuickSortSet org_lexemes_;
    // LexemePath position index table
    IKMap<size_t, std::unique_ptr<LexemePath>> path_map_;
    // Final tokenization result set
    IKList<Lexeme> results_;
    // Tokenizer configuration
    std::shared_ptr<Configuration> config_;

    void outputSingleCJK(size_t index);
    void compound(Lexeme& lexeme);

public:
    const CharacterUtil::TypedRuneArray& getTypedRuneArray() const { return typed_runes_; }
    explicit AnalyzeContext(std::shared_ptr<Configuration> configuration);
    virtual ~AnalyzeContext();

    void reset();

    size_t getCursor() const { return cursor_; }
    const char* getSegmentBuff() const { return segment_buff_.c_str(); }
    size_t getBufferOffset() const { return buffer_offset_; }
    size_t getLastUselessCharNum() const { return last_useless_char_num_; }

    size_t fillBuffer(lucene::util::Reader* reader);
    bool moveCursor();
    void initCursor();
    bool isBufferConsumed() const;
    bool needRefillBuffer() const;
    void markBufferOffset();

    void lockBuffer(const std::string& segmenterName);
    void unlockBuffer(const std::string& segmenterName);
    bool isBufferLocked() const;

    void addLexeme(Lexeme lexeme);
    void addLexemePath(std::unique_ptr<LexemePath> path);
    std::optional<Lexeme> getNextLexeme();
    QuickSortSet* getOrgLexemes() { return &org_lexemes_; }
    void outputToResult();

    size_t getCurrentCharLen() const {
        return cursor_ < typed_runes_.size() ? typed_runes_[cursor_].len : 0;
    }

    size_t getCurrentCharOffset() const {
        return cursor_ < typed_runes_.size() ? typed_runes_[cursor_].offset : 0;
    }

    int32_t getCurrentCharType() const { return typed_runes_[cursor_].char_type; }

    int32_t getCurrentChar() const {
        return cursor_ < typed_runes_.size() ? typed_runes_[cursor_].getChar() : 0;
    }
};

CL_NS_END2
#endif //CLUCENE_ANALYZECONTEXT_H
