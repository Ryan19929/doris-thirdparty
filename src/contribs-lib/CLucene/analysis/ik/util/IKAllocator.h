#ifndef CLUCENE_IKALLOCATOR_H
#define CLUCENE_IKALLOCATOR_H
#include <cstddef>
#include <new>
#include <vector>
#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

template<typename T, size_t BlockSize = 16>
class IKAllocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U>
    struct rebind { typedef IKAllocator<U, BlockSize> other; };


private:

    // 对齐的节点结构
    struct alignas(std::max_align_t) Node {
        alignas(T) char data[sizeof(T)];
        Node* next;
    };
    // 固定大小的块数组
    static constexpr size_t MAX_BLOCKS = 64;
    Node* blocks_[MAX_BLOCKS];
    size_t block_count_ = 0;

    // 预分配的内存池
    static constexpr size_t POOL_SIZE = BlockSize * 4;
    alignas(Node) char initial_pool_[sizeof(Node) * POOL_SIZE];
    bool pool_initialized_ = false;

    // 空闲列表
    Node* free_list_;

public:
    IKAllocator() : free_list_(nullptr) {
        initializePool();
    }

    IKAllocator(const IKAllocator&) : free_list_(nullptr) {
        initializePool();
    }

    template<typename U>
    IKAllocator(const IKAllocator<U, BlockSize>&) : free_list_(nullptr) {
        initializePool();
    }

    ~IKAllocator() {
        // 只释放动态分配的块
        for (size_t i = 0; i < block_count_; ++i) {
            if (blocks_[i] >= reinterpret_cast<Node*>(initial_pool_) &&
                blocks_[i] < reinterpret_cast<Node*>(initial_pool_ + sizeof(initial_pool_))) {
                continue;  // 跳过初始池的内存
            }
            ::operator delete(blocks_[i]);
        }
    }

private:
    void initializePool() {
        if (pool_initialized_) return;

        Node* nodes = reinterpret_cast<Node*>(initial_pool_);
        // 初始化预分配池中的节点
        for (size_t i = 0; i < POOL_SIZE - 1; ++i) {
            nodes[i].next = &nodes[i + 1];
        }
        nodes[POOL_SIZE - 1].next = nullptr;
        free_list_ = nodes;

        blocks_[0] = nodes;
        block_count_ = 1;
        pool_initialized_ = true;
    }

    // 分配新的内存块
    Node* allocateBlock() {
        if (block_count_ >= MAX_BLOCKS) {
            throw std::bad_alloc();
        }

        Node* block = static_cast<Node*>(::operator new(sizeof(Node) * BlockSize));
        blocks_[block_count_++] = block;

        // 初始化新块中的节点
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

        if (!free_list_) {
            free_list_ = allocateBlock();
        }

        Node* result = free_list_;
        free_list_ = free_list_->next;
        return reinterpret_cast<pointer>(&result->data);
    }

    void deallocate(pointer p, size_type n) {
        if (n != 1) {
            ::operator delete(p);
            return;
        }

        // 快速路径 - 直接头插法
        Node* node = reinterpret_cast<Node*>(
                reinterpret_cast<char*>(p) - offsetof(Node, data)
        );
        node->next = free_list_;
        free_list_ = node;
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

    bool operator==(const IKAllocator&) const { return true; }
    bool operator!=(const IKAllocator&) const { return false; }
};

CL_NS_END2


#endif //CLUCENE_IKALLOCATOR_H
