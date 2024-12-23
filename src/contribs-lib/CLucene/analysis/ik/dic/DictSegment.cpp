#include "DictSegment.h"

#include <algorithm>
#include <cassert>

#include "CLucene/_SharedHeader.h"

CL_NS_DEF2(analysis, ik)

DictSegment::DictSegment(int32_t key_char) : key_char_(key_char) {
    children_array_.reserve(ARRAY_LENGTH_LIMIT);
    storage_type_ = StorageType::Array;
}

bool DictSegment::hasNextNode() const {
    switch (storage_type_) {
    case StorageType::Array:
        return !children_array_.empty();
    case StorageType::Map:
        return !children_map_.empty();
    case StorageType::Hybrid:
        return hybrid_map_ != nullptr;
    default:
        return false;
    }
}

Hit DictSegment::match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
                       size_t count) {
    Hit search_hit;
    search_hit.setByteBegin(typed_runes[unicode_offset].offset);
    search_hit.setCharBegin(unicode_offset);
    match(typed_runes, unicode_offset, count, search_hit);
    return search_hit;
}

void DictSegment::match(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
                        size_t count, Hit& search_hit) {
    search_hit.setUnmatch();
    if (unicode_offset >= typed_runes.size()) {
        return;
    }

    search_hit.setByteEnd(typed_runes[unicode_offset].offset +
                          typed_runes[unicode_offset].getByteLength());
    search_hit.setCharEnd(unicode_offset);

    count = (count == 0 || unicode_offset + count > typed_runes.size())
                    ? typed_runes.size() - unicode_offset
                    : count;

    int32_t ch = typed_runes[unicode_offset].getChar();
    DictSegment* ds = nullptr;

    switch (storage_type_) {
    case StorageType::Array: {
        for (size_t i = 0; i < store_size_ && i < children_array_.size(); i++) {
            if (children_array_[i]->key_char_ == ch) {
                ds = children_array_[i].get();
                break;
            }
        }
        break;
    }

    case StorageType::Hybrid: {
        if (hybrid_map_) {
            ds = hybrid_map_->find(ch);
        }
        break;
    }

    case StorageType::Map: {
        auto it = children_map_.find(ch);
        if (it != children_map_.end()) {
            ds = it->second.get();
        }
        break;
    }
    }

    if (ds) {
        if (count == 1) {
            bool is_match = (ds->node_state_ == 1);
            bool has_next = ds->hasNextNode();

            if (is_match) search_hit.setMatch();
            if (has_next) {
                search_hit.setPrefix();
                search_hit.setMatchedDictSegment(ds);
            }
        } else {
            ds->match(typed_runes, unicode_offset + 1, count - 1, search_hit);
        }
    }
}

void DictSegment::fillSegment(const char* text) {
    if (!text) return;

    std::string cleanText = text;
    if (!cleanText.empty() && static_cast<unsigned char>(cleanText[0]) == 0xEF &&
        static_cast<unsigned char>(cleanText[1]) == 0xBB &&
        static_cast<unsigned char>(cleanText[2]) == 0xBF) {
        cleanText.erase(0, 3); // 移除 UTF-8 BOM
    }
    if (!cleanText.empty() && cleanText.back() == '\r') {
        cleanText.pop_back();
    }

    if (cleanText.empty()) return;

    CharacterUtil::RuneStrArray runes;
    if (!CharacterUtil::decodeString(cleanText.c_str(), cleanText.length(), runes)) {
        return;
    }

    DictSegment* current = this;
    for (const auto& rune : runes) {
        auto child = current->lookforSegment(rune.rune, true);
        if (!child) return;
        current = child;
    }
    current->node_state_ = 1;
}

DictSegment* DictSegment::lookforSegment(int32_t keyChar, bool create) {
    DictSegment* child = nullptr;

    if (store_size_ <= ARRAY_LENGTH_LIMIT) {
        for (size_t i = 0; i < store_size_ && i < children_array_.size(); i++) {
            if (children_array_[i]->key_char_ == keyChar) {
                child = children_array_[i].get();
                break;
            }
        }
        if (!child && create) {
            auto new_segment = std::make_unique<DictSegment>(keyChar);
            if (store_size_ < ARRAY_LENGTH_LIMIT) {
                if (store_size_ >= children_array_.size()) {
                    children_array_.resize(store_size_ + 1);
                }
                // 插入并保持有序
                size_t insertPos = 0;
                while (insertPos < store_size_ && children_array_[insertPos]->key_char_ < keyChar) {
                    insertPos++;
                }
                // 移动现有元素
                for (size_t i = store_size_; i > insertPos; i--) {
                    children_array_[i] = std::move(children_array_[i - 1]);
                }
                child = new_segment.get();

                children_array_[insertPos] = std::move(new_segment);
                store_size_++;
                storage_type_ = StorageType::Array;
            } else {
                for (size_t i = 0; i < store_size_; i++) {
                    children_map_[children_array_[i]->key_char_] = std::move(children_array_[i]);
                }
                child = new_segment.get();
                children_map_[keyChar] = std::move(new_segment);
                store_size_++;
                children_array_.clear();
                children_array_.shrink_to_fit();
                storage_type_ = StorageType::Map;
            }
        }
    } else {
        // 大数据量存储逻辑
        switch (storage_type_) {
        case StorageType::Hybrid:
            if (hybrid_map_) {
                child = hybrid_map_->find(keyChar);
                if (!child && create) {
                    auto new_segment = std::make_unique<DictSegment>(keyChar);
                    child = new_segment.get();
                    hybrid_map_->insert(keyChar, std::move(new_segment));
                    store_size_++;
                }
            }
            break;

        case StorageType::Map:
            auto it = children_map_.find(keyChar);
            if (it != children_map_.end()) {
                child = it->second.get();
            } else if (create) {
                auto new_segment = std::make_unique<DictSegment>(keyChar);
                child = new_segment.get();
                children_map_[keyChar] = std::move(new_segment);
                store_size_++;

                if (store_size_ >= HYBRID_MAP_THRESHOLD) {
                    convertToHybridMap();
                }
            }
            break;
        }
    }

    return child;
}

DictSegment::NodeStats DictSegment::collectStats() const {
    NodeStats stats;
    stats.total_nodes = 1; // 当前节点
    stats.max_depth = 0;   // 从0开始计算深度

    if (node_state_ == 1) {
        stats.word_count = 1;
    }

    if (store_size_ <= ARRAY_LENGTH_LIMIT) {
        stats.array_nodes = 1;
        // 遍历数组中的子节点
        for (size_t i = 0; i < store_size_; ++i) {
            auto child_stats = children_array_[i]->collectStats();
            stats.total_nodes += child_stats.total_nodes;
            stats.array_nodes += child_stats.array_nodes;
            stats.map_nodes += child_stats.map_nodes;
            stats.max_map_size = std::max(stats.max_map_size, child_stats.max_map_size);
            stats.total_map_entries += child_stats.total_map_entries;
            stats.max_depth = std::max(stats.max_depth + 1, child_stats.max_depth);
            stats.word_count += child_stats.word_count;
        }
    } else {
        stats.map_nodes = 1;
        stats.max_map_size = std::max(stats.max_map_size, children_map_.size());
        stats.total_map_entries += children_map_.size();

        // 遍历map中的子节点
        for (const auto& pair : children_map_) {
            auto child_stats = pair.second->collectStats();
            stats.total_nodes += child_stats.total_nodes;
            stats.array_nodes += child_stats.array_nodes;
            stats.map_nodes += child_stats.map_nodes;
            stats.max_map_size = std::max(stats.max_map_size, child_stats.max_map_size);
            stats.total_map_entries += child_stats.total_map_entries;
            stats.max_depth = std::max(stats.max_depth + 1, child_stats.max_depth);
            stats.word_count += child_stats.word_count;
        }
    }

    return stats;
}

void DictSegment::convertToHybridMap() {
    if (storage_type_ != StorageType::Map || store_size_ < HYBRID_MAP_THRESHOLD) {
        return;
    }

    hybrid_map_ = std::make_unique<HybridMap>();

    for (auto& pair : children_map_) {
        hybrid_map_->insert(pair.first, std::move(pair.second));
    }

    children_map_.clear();
    storage_type_ = StorageType::Hybrid;
}

CL_NS_END2