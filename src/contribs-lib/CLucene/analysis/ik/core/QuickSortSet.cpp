#include "QuickSortSet.h"

#include "CLucene/util/Misc.h"

CL_NS_DEF2(analysis, ik)

QuickSortSet::Cell* QuickSortSet::free_list_ = nullptr;
QuickSortSet::Cell QuickSortSet::pool_[POOL_SIZE];
bool QuickSortSet::pool_initialized_ = false;

void QuickSortSet::initPool() {
    if (pool_initialized_) return;

    for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
        pool_[i].next_ = &pool_[i + 1];
    }
    pool_[POOL_SIZE - 1].next_ = nullptr;
    free_list_ = &pool_[0];
    pool_initialized_ = true;
}

QuickSortSet::Cell* QuickSortSet::allocateCell() {
    if (!pool_initialized_) {
        initPool();
    }

    if (free_list_) {
        Cell* cell = free_list_;
        free_list_ = free_list_->next_;
        cell->next_ = nullptr;
        cell->prev_ = nullptr;
        return cell;
    }

    return new Cell();
}

void QuickSortSet::deallocateCell(Cell* cell) {
    if (cell >= pool_ && cell < pool_ + POOL_SIZE) {
        cell->next_ = free_list_;
        free_list_ = cell;
    } else {
        delete cell;
    }
}

QuickSortSet::~QuickSortSet() {
    clear();
}

void QuickSortSet::clear() {
    while (head_) {
        Cell* next = head_->next_;
        deallocateCell(head_);
        head_ = next;
    }
    tail_ = nullptr;
    cell_size_ = 0;
}

bool QuickSortSet::addLexeme(Lexeme lexeme) {
    Cell* new_cell = allocateCell();
    new_cell->lexeme_ = std::move(lexeme);

    if (cell_size_ == 0) {
        head_ = tail_ = new_cell;
        cell_size_++;
        return true;
    }

    if (*tail_ == *new_cell) {
        deallocateCell(new_cell);
        return false;
    }

    if (*new_cell < *tail_) {
        tail_->next_ = new_cell;
        new_cell->prev_ = tail_;
        tail_ = new_cell;
        cell_size_++;
        return true;
    }

    if (*head_ < *new_cell) {
        head_->prev_ = new_cell;
        new_cell->next_ = head_;
        head_ = new_cell;
        cell_size_++;
        return true;
    }

    auto index = tail_;
    while (index && *index < *new_cell) {
        index = index->prev_;
    }

    if (index && *index == *new_cell) {
        deallocateCell(new_cell);
        return false;
    }

    if (index && *new_cell < *index) {
        new_cell->prev_ = index;
        if (auto next = index->next_) {
            new_cell->next_ = next;
            next->prev_ = new_cell;
        }
        index->next_ = new_cell;
        cell_size_++;
        return true;
    }
    deallocateCell(new_cell);

    return false;
}

const Lexeme* QuickSortSet::peekFirst() const {
    return head_ ? &head_->lexeme_ : nullptr;
}

std::optional<Lexeme> QuickSortSet::pollFirst() {
    if (!head_) return std::nullopt;

    std::optional<Lexeme> result(std::move(head_->lexeme_));

    Cell* old_head = head_;
    head_ = head_->next_;
    if (head_)
        head_->prev_ = nullptr;
    else
        tail_ = nullptr;

    deallocateCell(old_head);
    --cell_size_;
    return result;
}

const Lexeme* QuickSortSet::peekLast() const {
    return tail_ ? &tail_->lexeme_ : nullptr;
}

std::optional<Lexeme> QuickSortSet::pollLast() {
    if (!tail_) return std::nullopt;
    std::optional<Lexeme> result(std::move(tail_->lexeme_));
    Cell* old_tail = tail_;

    tail_ = tail_->prev_;
    if (tail_)
        tail_->next_ = nullptr;
    else
        head_ = nullptr;

    deallocateCell(old_tail);
    --cell_size_;
    return result;
}

size_t QuickSortSet::getPathBegin() const {
    return head_ ? head_->lexeme_.getByteBegin() : 0;
}

size_t QuickSortSet::getPathEnd() const {
    return tail_ ? tail_->lexeme_.getByteBegin() + tail_->lexeme_.getByteLength() : 0;
}

CL_NS_END2