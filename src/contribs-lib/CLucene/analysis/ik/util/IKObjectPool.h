#ifndef CLUCENE_IKOBJECTPOOL_H
#define CLUCENE_IKOBJECTPOOL_H

#include <array>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

/**
 * @brief Thread-safe object pool implementation with thread local storage
 * @tparam T The type of objects to pool
 */
template <typename T>
class IKObjectPool {
private:
    static constexpr size_t DEFAULT_CHUNK_SIZE = 1024;
    static constexpr size_t MAX_POOL_SIZE = 1024 * 1024;

    // 线程本地存储块结构
    struct LocalBlock {
        std::vector<T*> objects;
        size_t used = 0;

        explicit LocalBlock(size_t size) {
            objects.resize(size);
            for (size_t i = 0; i < size; i++) {
                objects[i] = new T();
            }
        }

        ~LocalBlock() {
            for (auto* obj : objects) {
                delete obj;
            }
        }
    };

    // 线程本地存储
    static thread_local std::unique_ptr<LocalBlock> local_block_;

    std::vector<std::unique_ptr<LocalBlock>> shared_blocks_;
    mutable std::mutex mutex_;
    const size_t allocation_chunk_size_;

    void grow() {
        std::lock_guard<std::mutex> lock(mutex_);
        shared_blocks_.push_back(std::make_unique<LocalBlock>(allocation_chunk_size_));
    }

    IKObjectPool(size_t allocation_chunk_size = DEFAULT_CHUNK_SIZE)
            : allocation_chunk_size_(allocation_chunk_size) {
        grow();
    }

public:
    ~IKObjectPool() = default;

    static IKObjectPool& getInstance() {
        static IKObjectPool instance;
        return instance;
    }

    T* acquire() noexcept(false) {
        // 优先使用线程本地存储
        if (!local_block_) {
            local_block_ = std::make_unique<LocalBlock>(allocation_chunk_size_);
        }

        if (local_block_->used < local_block_->objects.size()) {
            return local_block_->objects[local_block_->used++];
        }

        // 线程本地存储耗尽,从共享池获取
        std::lock_guard<std::mutex> lock(mutex_);
        if (shared_blocks_.empty()) {
            if (shared_blocks_.size() * allocation_chunk_size_ >= MAX_POOL_SIZE) {
                throw std::runtime_error("Pool size limit exceeded");
            }
            grow();
        }

        // 获取一个新的块作为线程本地存储
        local_block_ = std::move(shared_blocks_.back());
        shared_blocks_.pop_back();
        return local_block_->objects[local_block_->used++];
    }

    void release(T* obj) noexcept {
        if (!obj) return;
        try {
            obj->reset();
            // 优先放回线程本地存储
            if (local_block_ && local_block_->used > 0) {
                local_block_->used--;
                local_block_->objects[local_block_->used] = obj;
                return;
            }

            // 线程本地存储已满,放回共享池
            std::lock_guard<std::mutex> lock(mutex_);
            if (shared_blocks_.empty() ||
                shared_blocks_.back()->used >= shared_blocks_.back()->objects.size()) {
                grow();
            }
            shared_blocks_.back()->objects[shared_blocks_.back()->used++] = obj;
        } catch (...) {
            delete obj;
        }
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t total = 0;
        for (const auto& block : shared_blocks_) {
            total += block->objects.size();
        }
        if (local_block_) {
            total += local_block_->objects.size();
        }
        return total;
    }

    size_t chunk_size() const noexcept { return allocation_chunk_size_; }

    void clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        shared_blocks_.clear();
        local_block_.reset();
    }
};

// 初始化线程本地存储
template <typename T>
thread_local std::unique_ptr<typename IKObjectPool<T>::LocalBlock> IKObjectPool<T>::local_block_;

CL_NS_END2

#endif //CLUCENE_IKOBJECTPOOL_H
