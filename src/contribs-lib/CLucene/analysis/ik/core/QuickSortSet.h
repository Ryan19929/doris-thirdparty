#ifndef CLUCENE_QUICKSORTSET_H
#define CLUCENE_QUICKSORTSET_H

#include "CLucene/_ApiHeader.h"
#include "Lexeme.h"

CL_NS_DEF2(analysis, ik)

class IKArbitrator;
class CLUCENE_EXPORT QuickSortSet {
    friend class IKArbitrator;

protected:
    class Cell {
    public:
        explicit Cell(std::shared_ptr<Lexeme> lexeme) : lexeme_(std::move(lexeme)) {}

        ~Cell() = default;

        bool operator<(const Cell& other) const;
        bool operator==(const Cell& other) const;

        std::shared_ptr<Cell> getPrev() const { return prev_.lock(); }
        std::shared_ptr<Cell> getNext() const { return next_; }
        std::shared_ptr<Lexeme> getLexeme() const { return lexeme_; }

    private:
        std::weak_ptr<Cell> prev_;
        std::shared_ptr<Cell> next_;
        std::shared_ptr<Lexeme> lexeme_;

        friend class QuickSortSet;
    };

    std::shared_ptr<Cell> head_;
    std::shared_ptr<Cell> tail_;
    size_t cell_size_ {0};

public:
    QuickSortSet();
    virtual ~QuickSortSet();

    QuickSortSet(const QuickSortSet&) = delete;
    QuickSortSet& operator=(const QuickSortSet&) = delete;

    bool addLexeme(std::shared_ptr<Lexeme> lexeme);
    std::shared_ptr<Lexeme> peekFirst();
    std::shared_ptr<Lexeme> pollFirst();
    std::shared_ptr<Lexeme> peekLast();
    std::shared_ptr<Lexeme> pollLast();
    void clear();

    size_t getSize() const { return cell_size_; }
    bool isEmpty() const { return cell_size_ == 0; }
    std::shared_ptr<Cell> getHead() { return head_; }
    std::shared_ptr<const Cell> getHead() const { return head_; }
};

CL_NS_END2
#endif //CLUCENE_QUICKSORTSET_H
