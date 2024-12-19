#ifndef CLUCENE_ALLOCATORMANAGER_H
#define CLUCENE_ALLOCATORMANAGER_H

#include "IKAllocator.h"
#include <memory>
#include <deque>
#include <stack>
#include <type_traits>

CL_NS_DEF2(analysis, ik)



// 为标准容器定义使用 IKAllocator 的别名
template<typename T>
using IKVector = std::vector<T, IKAllocator<T>>;

template<typename T>
using IKDeque = std::deque<T, IKAllocator<T>>;

template<typename T, size_t N = 8>
class IKStack {
private:
    T data_[N];
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

    bool empty() const {
        return size_ == 0 && overflow_stack_.empty();
    }
};


template<typename K, typename V, typename Compare = std::less<K>>
using IKMap = std::map<K, V, Compare, IKAllocator<std::pair<const K, V>>>;

template<typename K, typename V, typename Hash = std::hash<K>>
using IKUnorderedMap = std::unordered_map<K, V, Hash, std::equal_to<K>, IKAllocator<std::pair<const K, V>>>;

template<typename T, typename Compare = std::less<T>>
using IKSet = std::set<T, Compare, IKAllocator<T>>;

template<typename T>
using IKList = std::list<T, IKAllocator<T>>;


CL_NS_END2

#endif //CLUCENE_ALLOCATORMANAGER_H
