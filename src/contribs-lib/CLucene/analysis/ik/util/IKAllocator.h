#ifndef CLUCENE_IKALLOCATOR_H
#define CLUCENE_IKALLOCATOR_H
#include <cstddef>
#include <new>
#include <vector>

#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

template <typename T, size_t BlockSize = 16>
class IKAllocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template <typename U>
    struct rebind {
        typedef IKAllocator<U, BlockSize> other;
    };

private:
    // Node structure with alignment
    struct alignas(std::max_align_t) Node {
        alignas(T) char data[sizeof(T)];
        Node* next;
    };
    // Pre-allocated memory pool
    static constexpr size_t POOL_SIZE = BlockSize * 4;
    alignas(Node) char initial_pool_[sizeof(Node) * POOL_SIZE];
    bool initialized_ = false;

    Node* free_list_head_;

    std::vector<Node*> blocks_;

public:
    IKAllocator() : free_list_head_(nullptr) {
        blocks_.reserve(64);
        initializePool();
    }

    IKAllocator(const IKAllocator&) : free_list_head_(nullptr) { initializePool(); }

    template <typename U>
    IKAllocator(const IKAllocator<U, BlockSize>&) : free_list_head_(nullptr) {
        initializePool();
    }

    ~IKAllocator() {
        // 清理所有分配的块
        for (Node* block : blocks_) {
            if (block >= reinterpret_cast<Node*>(initial_pool_) &&
                block < reinterpret_cast<Node*>(initial_pool_ + sizeof(initial_pool_))) {
                continue; // 跳过初始池的内存
            }
            ::operator delete(block);
        }
    }

private:
    void initializePool() {
        if (initialized_) return;

        Node* nodes = reinterpret_cast<Node*>(initial_pool_);
        // 初始化预分配池中的节点
        for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
            nodes[i].next = &nodes[i + 1];
        }
        nodes[POOL_SIZE - 1].next = nullptr;
        free_list_head_ = nodes;

        blocks_.push_back(nodes);
        initialized_ = true;
    }

    Node* allocateBlock() {
        Node* block = static_cast<Node*>(::operator new(sizeof(Node) * BlockSize));
        blocks_.push_back(block);

        for (size_t i = 0; i < BlockSize - 1; ++i) {
            block[i].next = &block[i + 1];
        }
        block[BlockSize - 1].next = nullptr;

        return block;
    }

public:
    pointer allocate(size_type n) {
        if (n != 1) {
            return static_cast<pointer>(::operator new(n * sizeof(T)));
        }

        if (!free_list_head_) {
            try {
                free_list_head_ = allocateBlock();
            } catch (const std::bad_alloc& e) {
                return static_cast<pointer>(::operator new(sizeof(T)));
            }
        }

        Node* result = free_list_head_;
        free_list_head_ = free_list_head_->next;
        return reinterpret_cast<pointer>(&result->data);
    }

    void deallocate(pointer p, size_type n) {
        if (!p) return;
        if (n != 1) {
            ::operator delete(p);
            return;
        }

        Node* node = reinterpret_cast<Node*>(reinterpret_cast<char*>(p) - offsetof(Node, data));
        node->next = free_list_head_;
        free_list_head_ = node;
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

    bool operator==(const IKAllocator&) const { return true; }
    bool operator!=(const IKAllocator&) const { return false; }

    size_t max_size() const noexcept { return std::numeric_limits<size_type>::max() / sizeof(T); }

    size_t get_allocated_block_count() const { return blocks_.size(); }
};

CL_NS_END2

#endif //CLUCENE_IKALLOCATOR_H
