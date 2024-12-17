#include "QuickSortSet.h"

#include "CLucene/util/Misc.h"

CL_NS_DEF2(analysis, ik)

bool QuickSortSet::Cell::operator<(const Cell& other) const {
    return *lexeme_ < *other.lexeme_;
}

bool QuickSortSet::Cell::operator==(const Cell& other) const {
    return *lexeme_ == *other.lexeme_;
}

QuickSortSet::QuickSortSet() = default;

QuickSortSet::~QuickSortSet() {
    clear();
}

void QuickSortSet::clear() {
    head_.reset();
    tail_.reset();
    cell_size_ = 0;
}

bool QuickSortSet::addLexeme(std::shared_ptr<Lexeme> lexeme) {
    if (!lexeme) {
        return false;
    }
    auto new_cell = std::make_shared<Cell>(std::move(lexeme));

    if (cell_size_ == 0) {
        head_ = tail_ = new_cell;
        cell_size_++;
        return true;
    }

    if (*tail_ == *new_cell) {
        return false;
    }

    //  词元插入到链表尾部
    if (*new_cell < *tail_) {
        tail_->next_ = new_cell;
        new_cell->prev_ = tail_;
        tail_ = new_cell;
        cell_size_++;
        return true;
    }

    // 词元插入到链表头部
    if (*head_ < *new_cell) {
        head_->prev_ = new_cell;
        new_cell->next_ = head_;
        head_ = new_cell;
        cell_size_++;
        return true;
    }

    // 从尾部上逆插入
    auto index = tail_;
    while (index && *index < *new_cell) {
        index = index->getPrev();
    }

    if (index && *index == *new_cell) {
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

    return false;
}

std::shared_ptr<Lexeme> QuickSortSet::peekFirst() {
    return head_ ? head_->getLexeme() : nullptr;
}

std::shared_ptr<Lexeme> QuickSortSet::pollFirst() {
    if (!head_) return nullptr;

    auto result = head_->getLexeme();
    auto nextCell = head_->getNext(); // 获取下一个节点的 shared_ptr

    head_ = nextCell;
    if (head_)
        head_->prev_.reset();
    else
        tail_.reset();

    --cell_size_;
    return result;
}

std::shared_ptr<Lexeme> QuickSortSet::peekLast() {
    return tail_ ? tail_->getLexeme() : nullptr;
}

std::shared_ptr<Lexeme> QuickSortSet::pollLast() {
    if (!tail_) return nullptr;
    auto result = tail_->getLexeme();
    auto prev_cell = tail_->prev_.lock();

    tail_ = prev_cell;
    if (tail_)
        tail_->next_.reset();
    else
        head_.reset();

    --cell_size_;
    return result;
}

CL_NS_END2