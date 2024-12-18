#ifndef CLUCENE_OBJECTPOOL_H
#define CLUCENE_OBJECTPOOL_H
#include <mutex>
#include <vector>

#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

template <typename T>
class ObjectPool {
private:
    std::vector<T*> free_objects_;
    std::mutex mutex_;
    const size_t chunk_size_;

    void grow() {
        size_t old_size = free_objects_.size();
        free_objects_.resize(old_size + chunk_size_, nullptr);
        for (size_t i = 0; i < chunk_size_; i++) {
            free_objects_[old_size + i] = new T();
        }
    }

    ObjectPool(size_t chunk_size = 1024) : chunk_size_(chunk_size) { grow(); }

public:
    ~ObjectPool() {
        for (auto* obj : free_objects_) {
            delete obj;
        }
    }

    static ObjectPool& getInstance() {
        static ObjectPool instance;
        return instance;
    }

    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (free_objects_.empty()) {
            grow();
        }
        T* obj = free_objects_.back();
        free_objects_.pop_back();
        return obj;
    }

    void release(T* obj) {
        if (!obj) return;
        obj->reset();
        std::lock_guard<std::mutex> lock(mutex_);
        free_objects_.push_back(obj);
    }
};

CL_NS_END2
#endif //CLUCENE_OBJECTPOOL_H
