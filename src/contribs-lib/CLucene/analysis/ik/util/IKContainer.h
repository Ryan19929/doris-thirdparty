#ifndef CLUCENE_IKCONTAINER_H
#define CLUCENE_IKCONTAINER_H

#include <deque>
#include <memory>
#include <stack>
#include <type_traits>

#include "IKAllocator.h"

CL_NS_DEF2(analysis, ik)

// 为标准容器定义使用 IKAllocator 的别名
template <typename T>
using IKVector = std::vector<T, IKAllocator<T>>; // Vector with IK memory pool

template <typename T>
using IKDeque = std::deque<T, IKAllocator<T>>; // Double-ended queue with IK memory pool

template <typename T, size_t N = 8>
class IKStack {
private:
    T data_[N]; // Fixed-size storage array
    size_t size_ = 0;
    std::stack<T, std::deque<T, IKAllocator<T>>> overflow_stack_;

public:
    void push(const T& value) {
        if (size_ < N) {
            data_[size_++] = value;
        } else {
            overflow_stack_.push(value);
        }
    }

    T top() const {
        if (!overflow_stack_.empty()) {
            return overflow_stack_.top();
        }
        return data_[size_ - 1];
    }

    void pop() {
        if (!overflow_stack_.empty()) {
            overflow_stack_.pop();
        } else if (size_ > 0) {
            --size_;
        }
    }

    bool empty() const { return size_ == 0 && overflow_stack_.empty(); }

    size_t size() const { return size_ + overflow_stack_.size(); }

    bool is_overflow() const { return !overflow_stack_.empty(); }
};

template <typename K, typename V, typename Compare = std::less<K>>
using IKMap = std::map<K, V, Compare, IKAllocator<std::pair<const K, V>>>;

template <typename K, typename V, typename Hash = std::hash<K>>
using IKUnorderedMap =
        std::unordered_map<K, V, Hash, std::equal_to<K>, IKAllocator<std::pair<const K, V>>>;

template <typename T, typename Compare = std::less<T>>
using IKSet = std::set<T, Compare, IKAllocator<T>>;

template <typename T>
using IKList = std::list<T, IKAllocator<T>>;

CL_NS_END2

#endif //CLUCENE_IKCONTAINER_H
