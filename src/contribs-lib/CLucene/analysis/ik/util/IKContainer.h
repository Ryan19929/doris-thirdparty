#ifndef CLUCENE_IKCONTAINER_H
#define CLUCENE_IKCONTAINER_H

#include <deque>
#include <memory>
#include <stack>
#include <type_traits>

#include "IKAllocator.h"

CL_NS_DEF2(analysis, ik)

// Define a vector type that uses the custom IKAllocator for memory management
template <typename T>
using IKVector = std::vector<T, IKAllocator<T>>;

// Define a deque type that uses the custom IKAllocator for memory management
template <typename T>
using IKDeque = std::deque<T, IKAllocator<T>>;

// A stack implementation that uses a custom allocator and supports dynamic resizing
template <typename T, size_t InitialSize = 32>
class IKStack {
private:
    T* data_;                        // Pointer to the stack's data
    size_t capacity_;                // Current capacity of the stack
    size_t size_;                    // Current size of the stack
    T initial_storage_[InitialSize]; // Initial storage for small stacks

public:
    // Constructor initializes the stack with the initial storage
    IKStack() : size_(0), capacity_(InitialSize) { data_ = initial_storage_; }

    // Move constructor for transferring ownership of resources
    IKStack(IKStack&& other) noexcept : size_(other.size_), capacity_(other.capacity_) {
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

    // Move assignment operator for transferring ownership of resources
    IKStack& operator=(IKStack&& other) noexcept {
        if (this != &other) {
            if (data_ != initial_storage_) {
                delete[] data_; // Free previously allocated memory
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

    // Deleted copy constructor and copy assignment operator to prevent copying
    IKStack(const IKStack&) = delete;
    IKStack& operator=(const IKStack&) = delete;

    // Destructor to free allocated memory
    ~IKStack() {
        if (data_ != initial_storage_) {
            delete[] data_;
        }
    }

    // Push a new element onto the stack
    void push(const T& value) {
        if (size_ >= capacity_) {
            grow(); // Increase capacity if needed
        }
        data_[size_++] = value;
    }

    // Get the top element of the stack
    T top() const {
        assert(size_ > 0 && "Stack is empty");
        return data_[size_ - 1];
    }

    // Remove the top element from the stack
    void pop() {
        if (size_ > 0) {
            --size_;
            if (size_ < capacity_ / 4 && capacity_ > InitialSize) {
                shrink(); // Reduce capacity if needed
            }
        }
    }

    // Check if the stack is empty
    bool empty() const { return size_ == 0; }

    // Get the current size of the stack
    size_t size() const { return size_; }

private:
    // Increase the capacity of the stack
    void grow() {
        size_t new_capacity = capacity_ * 2; // Double the capacity
        T* new_data = new T[new_capacity];   // Allocate new memory

        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[i]; // Copy existing elements
        }

        if (data_ != initial_storage_) {
            delete[] data_; // Free old memory if it was dynamically allocated
        }

        data_ = new_data;         // Update data pointer
        capacity_ = new_capacity; // Update capacity
    }

    // Reduce the capacity of the stack
    void shrink() {
        size_t new_capacity = capacity_ / 2; // Halve the capacity
        if (new_capacity < InitialSize) {
            if (data_ != initial_storage_) {
                for (size_t i = 0; i < size_; ++i) {
                    initial_storage_[i] = data_[i]; // Move data to initial storage
                }
                delete[] data_;           // Free old memory
                data_ = initial_storage_; // Reset to initial storage
                capacity_ = InitialSize;  // Reset capacity
            }
        } else {
            T* new_data = new T[new_capacity]; // Allocate new memory
            for (size_t i = 0; i < size_; ++i) {
                new_data[i] = data_[i]; // Copy existing elements
            }
            if (data_ != initial_storage_) {
                delete[] data_; // Free old memory
            }
            data_ = new_data;         // Update data pointer
            capacity_ = new_capacity; // Update capacity
        }
    }
};

// Define a map type that uses the custom IKAllocator for memory management
template <typename K, typename V, typename Compare = std::less<K>>
using IKMap = std::map<K, V, Compare, IKAllocator<std::pair<const K, V>>>;

// Define an unordered map type that uses the custom IKAllocator for memory management
template <typename K, typename V, typename Hash = std::hash<K>>
using IKUnorderedMap =
        std::unordered_map<K, V, Hash, std::equal_to<K>, IKAllocator<std::pair<const K, V>>>;

// Define a set type that uses the custom IKAllocator for memory management
template <typename T, typename Compare = std::less<T>>
using IKSet = std::set<T, Compare, IKAllocator<T>>;

// Define a list type that uses the custom IKAllocator for memory management
template <typename T>
using IKList = std::list<T, IKAllocator<T>>;

CL_NS_END2

#endif //CLUCENE_IKCONTAINER_H
