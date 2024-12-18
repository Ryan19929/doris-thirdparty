#ifndef CLUCENE_DICTSEGMENT_H
#define CLUCENE_DICTSEGMENT_H

#include <CLucene.h>

#include <unordered_map>
#include <memory>
#include <vector>

#include "CLucene/analysis/ik/core/CharacterUtil.h"
#include "Hit.hpp"
CL_NS_DEF2(analysis, ik)

class CLUCENE_EXPORT DictSegment : public std::enable_shared_from_this<DictSegment> {
private:
    static const size_t ARRAY_LENGTH_LIMIT = 3;

    int32_t key_char_;
    std::vector<std::shared_ptr<DictSegment>> children_array_;
    std::unordered_map<int32_t, std::shared_ptr<DictSegment>> children_map_;
    size_t store_size_ {0};
    int node_state_ {0};

    std::shared_ptr<DictSegment> lookforSegment(int32_t key_char, bool create_if_missing);

public:
    // 添加统计结构体
    struct CollisionStats {
        size_t total_nodes {0};  // 总节点数
        size_t array_nodes {0};  // 使用数组存储的节点数
        size_t map_nodes {0};    // 使用哈希表存储的节点数
        std::map<size_t, size_t> bucket_distribution; // 哈希桶分布
        size_t max_collision {0}; // 最大碰撞数
        double avg_load_factor {0.0}; // 平均负载因子
    };

    // 收集统计信息的方法
    void collectStats(CollisionStats& stats) const {
        stats.total_nodes++;

        if(store_size_ <= ARRAY_LENGTH_LIMIT) {
            stats.array_nodes++;
        } else {
            stats.map_nodes++;
            // 统计哈希表信息
            for(size_t i = 0; i < children_map_.bucket_count(); i++) {
                size_t bucket_size = children_map_.bucket_size(i);
                if(bucket_size > 0) {
                    stats.bucket_distribution[bucket_size]++;
                    stats.max_collision = std::max(stats.max_collision, bucket_size);
                }
            }
            stats.avg_load_factor += children_map_.load_factor();
        }

        // 递归统计子节点
        if(store_size_ <= ARRAY_LENGTH_LIMIT) {
            for(const auto& child : children_array_) {
                if(child) {
                    child->collectStats(stats);
                }
            }
        } else {
            for(const auto& [_, child] : children_map_) {
                if(child) {
                    child->collectStats(stats);
                }
            }
        }
    }
    explicit DictSegment(int32_t key_char);
    ~DictSegment() = default;

    DictSegment(const DictSegment&) = delete;
    DictSegment& operator=(const DictSegment&) = delete;
    DictSegment(DictSegment&&) noexcept = default;
    DictSegment& operator=(DictSegment&&) noexcept = default;

    bool hasNextNode() const;
    Hit match(const CharacterUtil::TypedRuneArray& typed_runes);

    Hit match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
              size_t count);

    void match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
               size_t count, Hit& search_hit);

    void fillSegment(const char* text);
};

CL_NS_END2
#endif //CLUCENE_DICTSEGMENT_H
