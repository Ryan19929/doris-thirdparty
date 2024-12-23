#ifndef CLUCENE_QUICKSORTSET_H
#define CLUCENE_QUICKSORTSET_H

#include <memory>
#include <optional>

#include "CLucene/_ApiHeader.h"
#include "Lexeme.h"

CL_NS_DEF2(analysis, ik)

class IKArbitrator;
class CLUCENE_EXPORT QuickSortSet {
    friend class IKArbitrator;

protected:
    class Cell {
    public:
        Cell() = default;
        explicit Cell(Lexeme&& lexeme) : lexeme_(std::move(lexeme)) {}
        ~Cell() = default;

        bool operator<(const Cell& other) const { return lexeme_ < other.lexeme_; };
        bool operator==(const Cell& other) const { return lexeme_ == other.lexeme_; };

        Cell* getPrev() const { return prev_; }
        Cell* getNext() const { return next_; }
        const Lexeme& getLexeme() const { return lexeme_; }
        Lexeme& getLexeme() { return lexeme_; }

    private:
        Cell* prev_ = nullptr;
        Cell* next_ = nullptr;
        Lexeme lexeme_;

        friend class QuickSortSet;
    };

    Cell* head_ = nullptr;
    Cell* tail_ = nullptr;
    size_t cell_size_ = 0;

    static constexpr size_t POOL_SIZE = 1024; // 池大小，可以根据需要调整
    static Cell* free_list_;                  // 空闲Cell链表
    static Cell pool_[POOL_SIZE];             // 预分配的Cell池
    static bool pool_initialized_;            // 池初始化标志

    // 内存分配相关方法
    static void initPool();
    static Cell* allocateCell();
    static void deallocateCell(Cell* cell);

public:
    QuickSortSet() = default;
    virtual ~QuickSortSet();

    QuickSortSet(const QuickSortSet&) = delete;
    QuickSortSet& operator=(const QuickSortSet&) = delete;

    bool addLexeme(Lexeme lexeme);
    const Lexeme* peekFirst() const;
    std::optional<Lexeme> pollFirst();
    const Lexeme* peekLast() const;
    std::optional<Lexeme> pollLast();
    void clear();

    size_t getPathBegin() const;
    size_t getPathEnd() const;

    size_t getSize() const { return cell_size_; }
    bool isEmpty() const { return cell_size_ == 0; }
    Cell* getHead() { return head_; }
    const Cell* getHead() const { return head_; }
};

CL_NS_END2
#endif //CLUCENE_QUICKSORTSET_H
