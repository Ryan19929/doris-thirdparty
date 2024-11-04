#ifndef CLUCENE_LEXEME_H
#define CLUCENE_LEXEME_H

#include <stdexcept>
#include <string>

#include "CLucene/_ApiHeader.h"
#include "CharacterUtil.h"
CL_NS_DEF2(analysis, ik)

class CLUCENE_EXPORT Lexeme {
public:
    enum class Type {
        // Unknown
        Unknown = 0,
        // English
        English = 1,
        // Number
        Arabic = 2,
        // Mixed English and numbers
        Letter = 3,
        // Chinese word
        CNWord = 4,
        // Single Chinese character
        CNChar = 64,
        // CJK characters (Japanese and Korean)
        OtherCJK = 8,
        // Chinese numeric word
        CNum = 16,
        // Chinese measure word
        Count = 32,
        // Chinese numeric-measure compound
        CQuan = 48
    };

    explicit Lexeme(size_t offset, size_t begin, size_t length, Type type, size_t charBegin,
                    size_t charEnd);

    bool append(const Lexeme& other, Type newType);

    size_t getOffset() const noexcept { return offset_; }
    size_t getByteBegin() const noexcept { return byte_begin_; }
    size_t getCharBegin() const noexcept { return char_begin_; }
    size_t getByteLength() const noexcept { return byte_length_; }
    size_t getCharLength() const noexcept { return char_end_ - char_begin_ + 1; }
    Type getType() const noexcept { return type_; }
    const std::string& getText() const noexcept { return text_; }
    size_t getCharEnd() const noexcept { return char_end_; }

    size_t getByteBeginPosition() const noexcept { return offset_ + byte_begin_; }
    size_t getByteEndPosition() const noexcept { return offset_ + byte_begin_ + byte_length_; }

    void setLength(size_t length);
    void setType(Type type) noexcept { this->type_ = type; }
    void setText(std::string text);

    std::string toString() const;

    bool operator==(const Lexeme& other) const noexcept;
    bool operator<(const Lexeme& other) const noexcept;

private:
    size_t offset_;      // Byte offset
    size_t byte_begin_;  // Byte start position
    size_t byte_length_; // Lexeme length
    size_t char_begin_;  // Lexeme character start
    size_t char_end_;    // Lexeme character end
    std::string text_;   // UTF-8 text
    Type type_;          // Lexeme type
};

CL_NS_END2
#endif //CLUCENE_LEXEME_H
