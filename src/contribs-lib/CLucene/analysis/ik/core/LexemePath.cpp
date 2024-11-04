#include "LexemePath.h"

#include <sstream>

CL_NS_DEF2(analysis, ik)

LexemePath::LexemePath() : path_begin_(-1), path_end_(-1), payload_length_(0) {}

bool LexemePath::addCrossLexeme(const std::shared_ptr<Lexeme>& lexeme) {
    if (isEmpty()) {
        addLexeme(lexeme);
        path_begin_ = lexeme->getByteBegin();
        path_end_ = lexeme->getByteBegin() + lexeme->getByteLength();
        payload_length_ += lexeme->getByteLength();
        return true;
    }

    if (checkCross(lexeme)) {
        addLexeme(lexeme);
        if (lexeme->getByteBegin() + lexeme->getByteLength() > path_end_) {
            path_end_ = lexeme->getByteBegin() + lexeme->getByteLength();
        }
        payload_length_ = path_end_ - path_begin_;
        return true;
    }

    return false;
}

bool LexemePath::addNotCrossLexeme(const std::shared_ptr<Lexeme>& lexeme) {
    if (isEmpty()) {
        addLexeme(lexeme);
        path_begin_ = lexeme->getByteBegin();
        path_end_ = lexeme->getByteBegin() + lexeme->getByteLength();
        payload_length_ += lexeme->getByteLength();
        return true;
    }

    if (checkCross(lexeme)) {
        return false;
    }

    addLexeme(lexeme);
    payload_length_ += lexeme->getByteLength();
    auto head = peekFirst();
    path_begin_ = head->getByteBegin();
    auto tail = peekLast();
    path_end_ = tail->getByteBegin() + tail->getByteLength();
    return true;
}
std::shared_ptr<Lexeme> LexemePath::removeTail() {
    auto tail = pollLast();
    if (!tail) {
        return nullptr;
    }

    if (isEmpty()) {
        path_begin_ = -1;
        path_end_ = -1;
        payload_length_ = 0;
    } else {
        payload_length_ -= tail->getByteLength();
        auto newTail = peekLast();
        if (newTail) {
            path_end_ = newTail->getByteBegin() + newTail->getByteLength();
        }
    }
    return tail;
}

bool LexemePath::checkCross(const std::shared_ptr<Lexeme>& lexeme) const {
    return (lexeme->getByteBegin() >= path_begin_ && lexeme->getByteBegin() < path_end_) ||
           (path_begin_ >= lexeme->getByteBegin() &&
            path_begin_ < lexeme->getByteBegin() + lexeme->getByteLength());
}

size_t LexemePath::getXWeight() const {
    size_t product = 1;
    auto c = getHead();
    while (c != nullptr) {
        product *= c->getLexeme()->getByteLength();
        c = c->getNext();
    }
    return product;
}

size_t LexemePath::getPWeight() const {
    size_t pWeight = 0;
    size_t p = 0;
    auto c = getHead();
    while (c != nullptr) {
        p++;
        pWeight += p * c->getLexeme()->getByteLength();
        c = c->getNext();
    }
    return pWeight;
}

LexemePath* LexemePath::copy() const {
    LexemePath* theCopy = _CLNEW LexemePath();
    theCopy->path_begin_ = this->path_begin_;
    theCopy->path_end_ = this->path_end_;
    theCopy->payload_length_ = this->payload_length_;
    auto c = this->getHead();
    while (c != nullptr) {
        theCopy->addLexeme(c->getLexeme());
        c = c->getNext();
    }
    return theCopy;
}

bool LexemePath::operator<(const LexemePath& o) const {
    if (payload_length_ > o.payload_length_) return true;
    if (payload_length_ < o.payload_length_) return false;

    if (getSize() < o.getSize()) return true;
    if (getSize() > o.getSize()) return false;

    if (getPathLength() > o.getPathLength()) return true;
    if (getPathLength() < o.getPathLength()) return false;

    if (path_end_ > o.path_end_) return true;
    if (path_end_ < o.path_end_) return false;

    if (getXWeight() > o.getXWeight()) return true;
    if (getXWeight() < o.getXWeight()) return false;

    return getPWeight() > o.getPWeight();
}

bool LexemePath::operator==(const LexemePath& o) const {
    return path_begin_ == o.path_begin_ && path_end_ == o.path_end_ &&
           payload_length_ == o.payload_length_;
}

CL_NS_END2