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

template <typename T, size_t InitialSize = 32>
class IKStack {
private:
    T* data_;                        // 动态数组
    size_t capacity_;                // 当前容量
    size_t size_;                    // 当前大小
    T initial_storage_[InitialSize]; // 初始固定大小存储

public:
    IKStack() : size_(0), capacity_(InitialSize) {
        // 开始时使用固定大小数组
        data_ = initial_storage_;
    }

    // 添加移动构造函数
    IKStack(IKStack&& other) noexcept : size_(other.size_), capacity_(other.capacity_) {
        if (other.data_ == other.initial_storage_) {
            // 如果other使用的是初始存储，我们需要复制数据
            data_ = initial_storage_;
            for (size_t i = 0; i < size_; ++i) {
                initial_storage_[i] = other.initial_storage_[i];
            }
        } else {
            // 如果other使用的是堆内存，我们可以直接移动指针
            data_ = other.data_;
            other.data_ = other.initial_storage_;
        }
        other.size_ = 0;
        other.capacity_ = InitialSize;
    }

    // 添加移动赋值运算符
    IKStack& operator=(IKStack&& other) noexcept {
        if (this != &other) {
            if (data_ != initial_storage_) {
                delete[] data_;
            }

            size_ = other.size_;
            capacity_ = other.capacity_;

            if (other.data_ == other.initial_storage_) {
                data_ = initial_storage_;
                for (size_t i = 0; i < size_; ++i) {
                    initial_storage_[i] = other.initial_storage_[i];
                }
            } else {
                data_ = other.data_;
                other.data_ = other.initial_storage_;
            }

            other.size_ = 0;
            other.capacity_ = InitialSize;
        }
        return *this;
    }

    // 保持拷贝构造和拷贝赋值被禁用
    IKStack(const IKStack&) = delete;
    IKStack& operator=(const IKStack&) = delete;

    ~IKStack() {
        // 只有在使用堆内存时才需要释放
        if (data_ != initial_storage_) {
            delete[] data_;
        }
    }

    void push(const T& value) {
        if (size_ >= capacity_) {
            grow();
        }
        data_[size_++] = value;
    }

    T top() const {
        assert(size_ > 0 && "Stack is empty");
        return data_[size_ - 1];
    }

    void pop() {
        if (size_ > 0) {
            --size_;
            // 可选：当size显著小于capacity时，考虑收缩
            if (size_ < capacity_ / 4 && capacity_ > InitialSize) {
                shrink();
            }
        }
    }

    bool empty() const { return size_ == 0; }

    size_t size() const { return size_; }

private:
    void grow() {
        size_t new_capacity = capacity_ * 2;
        T* new_data = new T[new_capacity];

        // 复制现有数据
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[i];
        }

        // 如果之前使用的是堆内存，释放它
        if (data_ != initial_storage_) {
            delete[] data_;
        }

        data_ = new_data;
        capacity_ = new_capacity;
    }

    void shrink() {
        size_t new_capacity = capacity_ / 2;
        if (new_capacity < InitialSize) {
            // 如果新容量小于初始容量，回到使用固定数组
            if (data_ != initial_storage_) {
                for (size_t i = 0; i < size_; ++i) {
                    initial_storage_[i] = data_[i];
                }
                delete[] data_;
                data_ = initial_storage_;
                capacity_ = InitialSize;
            }
        } else {
            // 分配新的更小的空间
            T* new_data = new T[new_capacity];
            for (size_t i = 0; i < size_; ++i) {
                new_data[i] = data_[i];
            }
            if (data_ != initial_storage_) {
                delete[] data_;
            }
            data_ = new_data;
            capacity_ = new_capacity;
        }
    }
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
