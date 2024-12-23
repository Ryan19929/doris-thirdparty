#ifndef CLUCENE_IKOBJECTPOOL_H
#define CLUCENE_IKOBJECTPOOL_H

#include <mutex>
#include <stdexcept>
#include <vector>

#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

/**
 * @brief Thread-safe object pool implementation
 * @tparam T The type of objects to pool
 */
template <typename T>
class IKObjectPool {
private:
    static constexpr size_t DEFAULT_CHUNK_SIZE = 1024;
    static constexpr size_t MAX_POOL_SIZE = 1024 * 1024;

    IKObjectPool(const IKObjectPool&) = delete;
    IKObjectPool& operator=(const IKObjectPool&) = delete;
    IKObjectPool(IKObjectPool&&) = delete;
    IKObjectPool& operator=(IKObjectPool&&) = delete;

    std::vector<T*> available_objects_;
    mutable std::mutex mutex_;
    const size_t allocation_chunk_size_;

    void grow() {
        size_t old_size = available_objects_.size();
        available_objects_.resize(old_size + allocation_chunk_size_, nullptr);
        for (size_t i = 0; i < allocation_chunk_size_; i++) {
            available_objects_[old_size + i] = new T();
        }
    }

    IKObjectPool(size_t allocation_chunk_size = DEFAULT_CHUNK_SIZE)
            : allocation_chunk_size_(allocation_chunk_size) {
        grow();
    }

public:
    ~IKObjectPool() {
        for (auto* obj : available_objects_) {
            delete obj;
        }
    }

    static IKObjectPool& getInstance() {
        static IKObjectPool instance;
        return instance;
    }

    T* acquire() noexcept(false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_objects_.empty()) {
            if (available_objects_.size() >= MAX_POOL_SIZE) {
                throw std::runtime_error("Pool size limit exceeded");
            }
            grow();
        }
        T* obj = available_objects_.back();
        available_objects_.pop_back();
        return obj;
    }

    void release(T* obj) noexcept {
        if (!obj) return;
        try {
            obj->reset();
            std::lock_guard<std::mutex> lock(mutex_);
            available_objects_.push_back(obj);
        } catch (...) {
            delete obj;
        }
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_objects_.size();
    }

    size_t chunk_size() const noexcept { return allocation_chunk_size_; }

    void clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* obj : available_objects_) {
            delete obj;
        }
        available_objects_.clear();
    }
};

CL_NS_END2

#endif //CLUCENE_IKOBJECTPOOL_H
