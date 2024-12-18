#ifndef CLUCENE_OBJECTALLOCATOR_H
#define CLUCENE_OBJECTALLOCATOR_H
#include <cstddef>
#include <new>
#include <vector>
#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

template<typename T, size_t BlockSize = 16>
class SmallObjectAllocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U>
    struct rebind { typedef SmallObjectAllocator<U, BlockSize> other; };

private:
    struct Node {
        char data[sizeof(T)];
        Node* next;
    };

    Node* free_list_;
    std::vector<Node*> allocated_blocks_; // 跟踪已分配的内存块

public:
    SmallObjectAllocator() : free_list_(nullptr) {}

    // 复制构造函数
    SmallObjectAllocator(const SmallObjectAllocator&) : free_list_(nullptr) {}

    template<typename U>
    SmallObjectAllocator(const SmallObjectAllocator<U, BlockSize>&) : free_list_(nullptr) {}

    ~SmallObjectAllocator() {
        // 释放所有分配的内存块
        for (Node* block : allocated_blocks_) {
            ::operator delete(block);
        }
    }

    pointer allocate(size_type n) {
        if (n != 1) return static_cast<pointer>(::operator new(n * sizeof(T)));

        if (!free_list_) {
            // 一次性分配一个块
            Node* block = static_cast<Node*>(::operator new(sizeof(Node) * BlockSize));
            allocated_blocks_.push_back(block); // 记录分配的块

            for (size_t i = 0; i < BlockSize - 1; ++i) {
                block[i].next = &block[i + 1];
            }
            block[BlockSize - 1].next = nullptr;
            free_list_ = block;
        }

        Node* result = free_list_;
        free_list_ = free_list_->next;
        return reinterpret_cast<pointer>(result);
    }

    void deallocate(pointer p, size_type n) {
        if (n != 1) {
            ::operator delete(p);
            return;
        }

        // 检查指针是否在我们分配的块中
        Node* node = reinterpret_cast<Node*>(p);
        for (Node* block : allocated_blocks_) {
            if (node >= block && node < block + BlockSize) {
                node->next = free_list_;
                free_list_ = node;
                return;
            }
        }
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

    bool operator==(const SmallObjectAllocator&) const { return true; }
    bool operator!=(const SmallObjectAllocator&) const { return false; }
};

CL_NS_END2


#endif //CLUCENE_OBJECTALLOCATOR_H
