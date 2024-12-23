#ifndef CLUCENE_DICTSEGMENT_H
#define CLUCENE_DICTSEGMENT_H

#include <CLucene.h>

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "CLucene/analysis/ik/core/CharacterUtil.h"
#include "Hit.hpp"

CL_NS_DEF2(analysis, ik)

class CLUCENE_EXPORT DictSegment : public std::enable_shared_from_this<DictSegment> {
private:
    static constexpr size_t ARRAY_LENGTH_LIMIT = 8;      // 增加数组上限
    static constexpr size_t HYBRID_MAP_THRESHOLD = 1000; // 超过此值使用混合索引
    static constexpr size_t BUCKET_SIZE = 256;           // 混合索引桶大小

    // 优化的混合索引结构
    struct alignas(64) HybridMap { // 缓存行对齐
        struct Bucket {
            std::vector<std::pair<int32_t, std::unique_ptr<DictSegment>>> entries;

            DictSegment* find(int32_t key) const {
                if (entries.size() <= 8) { // 小数组线性查找
                    for (const auto& pair : entries) {
                        if (pair.first == key) {
                            return pair.second.get();
                        }
                    }
                    return nullptr;
                }
                // 大数组二分查找
                auto it = std::lower_bound(entries.begin(), entries.end(), key,
                                           [](const auto& p, int32_t k) { return p.first < k; });
                return (it != entries.end() && it->first == key) ? it->second.get() : nullptr;
            }

            void insert(int32_t key, std::unique_ptr<DictSegment> segment) {
                auto it = std::lower_bound(entries.begin(), entries.end(), key,
                                           [](const auto& p, int32_t k) { return p.first < k; });
                if (it == entries.end() || it->first != key) {
                    entries.emplace(it, key, std::move(segment));
                }
            }
        };

        std::array<Bucket, BUCKET_SIZE> buckets;

        size_t getBucketIndex(int32_t key) const {
            // 优化的bucket计算，考虑CJK字符范围
            if (key >= 0x4E00 && key <= 0x9FFF) { // CJK统一汉字
                return (key - 0x4E00) % BUCKET_SIZE;
            }
            return static_cast<size_t>(key) % BUCKET_SIZE;
        }

        DictSegment* find(int32_t key) const { return buckets[getBucketIndex(key)].find(key); }

        void insert(int32_t key, std::unique_ptr<DictSegment> segment) {
            buckets[getBucketIndex(key)].insert(key, std::move(segment));
        }
    };

    enum class StorageType : uint8_t { Array, Map, Hybrid };

    alignas(64) int32_t key_char_;                  // 缓存行对齐
    uint8_t node_state_ {0};                        // 词条标记
    StorageType storage_type_ {StorageType::Array}; // 存储类型
    size_t store_size_ {0};                         // 子节点数量

    std::vector<std::unique_ptr<DictSegment>> children_array_;
    std::unordered_map<int32_t, std::unique_ptr<DictSegment>> children_map_;
    std::unique_ptr<HybridMap> hybrid_map_;

    DictSegment* lookforSegment(int32_t key_char, bool create_if_missing);
    void convertToHybridMap();

public:
    struct NodeStats {
        size_t total_nodes {0};
        size_t array_nodes {0};
        size_t map_nodes {0};
        size_t hybrid_nodes {0};
        size_t max_map_size {0};
        size_t total_map_entries {0};
        size_t max_depth {0};
        size_t word_count {0};
    };

    explicit DictSegment(int32_t key_char);
    ~DictSegment() = default;

    DictSegment(const DictSegment&) = delete;
    DictSegment& operator=(const DictSegment&) = delete;
    DictSegment(DictSegment&&) noexcept = default;
    DictSegment& operator=(DictSegment&&) noexcept = default;

    bool hasNextNode() const;
    Hit match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
              size_t count);
    void match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
               size_t count, Hit& search_hit);
    void fillSegment(const char* text);
    NodeStats collectStats() const;
};

CL_NS_END2
#endif //CLUCENE_DICTSEGMENT_H
