#include "QuickSortSet.h"

#include "CLucene/util/Misc.h"

CL_NS_DEF2(analysis, ik)



QuickSortSet::~QuickSortSet() {
    clear();
}

void QuickSortSet::clear() {
    while (head_) {
        Cell* next = head_->next_;
        ObjectPool<Cell>::getInstance().release(head_);
        head_ = next;
    }
    tail_ = nullptr;
    cell_size_ = 0;
}

bool QuickSortSet::addLexeme(Lexeme lexeme) {
    auto* new_cell = ObjectPool<Cell>::getInstance().acquire();
//    auto new_cell = new Cell(std::move(lexeme));
    new_cell->lexeme_ = std::move(lexeme);

    if (cell_size_ == 0) {
        head_ = tail_ = new_cell;
        cell_size_++;
        return true;
    }

    if (*tail_ == *new_cell) {
        ObjectPool<Cell>::getInstance().release(new_cell);
//        delete new_cell;
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
        index = index->prev_;
    }

    if (index && *index == *new_cell) {
        //delete new_cell;
        ObjectPool<Cell>::getInstance().release(new_cell);
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

const Lexeme* QuickSortSet::peekFirst() const{
    return head_ ? &head_->lexeme_ : nullptr;
}

std::optional<Lexeme> QuickSortSet::pollFirst() {
    if (!head_) return std::nullopt;
    Cell* old_head = head_;
    Lexeme result = std::move(old_head->getLexeme());

    head_ = head_->next_;
    if (head_)
        head_->prev_ = nullptr;
    else
        tail_ = nullptr;

    ObjectPool<Cell>::getInstance().release(old_head);
    --cell_size_;
    return result;
}

const Lexeme* QuickSortSet::peekLast() const {
    return tail_ ? &tail_->lexeme_ : nullptr;
}

std::optional<Lexeme> QuickSortSet::pollLast() {
    if (!tail_) return std::nullopt;
    Cell* old_tail = tail_;
    Lexeme result = std::move(old_tail->getLexeme());

    tail_ = tail_->prev_;
    if (tail_)
        tail_->next_ = nullptr;
    else
        head_ = nullptr;

    ObjectPool<Cell>::getInstance().release(old_tail);
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