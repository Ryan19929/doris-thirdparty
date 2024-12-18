#include "DictSegment.h"

#include <algorithm>
#include <cassert>

#include "CLucene/_SharedHeader.h"

CL_NS_DEF2(analysis, ik)

DictSegment::DictSegment(int32_t key_char) : key_char_(key_char) {
    children_array_.reserve(ARRAY_LENGTH_LIMIT);
}

bool DictSegment::hasNextNode() const {
    return store_size_ > 0;
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

    if (!typed_runes.empty() && unicode_offset < typed_runes.size()) {
        search_hit.setByteEnd(typed_runes[unicode_offset].offset +
                              typed_runes[unicode_offset].getByteLength());
        search_hit.setCharEnd(unicode_offset);
    }

    count = (count == 0 || unicode_offset + count > typed_runes.size())
                    ? typed_runes.size() - unicode_offset
                    : count;

    int32_t ch = typed_runes[unicode_offset].getChar();
    DictSegment* ds = nullptr;
    if (store_size_ <= ARRAY_LENGTH_LIMIT) {
        for (size_t i = 0; i < store_size_ && i < children_array_.size(); i++) {
            if (children_array_[i]->key_char_ == ch) {
                ds = children_array_[i].get();
                break;
            }
        }
    } else {
        auto it = children_map_.find(ch);
        if (it != children_map_.end()) {
            ds = it->second.get();
        }
    }

    if (ds) {
        if (count > 1) {
            ds->match(typed_runes, unicode_offset + 1, count - 1, search_hit);
        } else if (count == 1) {
            if (ds->node_state_ == 1) {
                search_hit.setMatch();
            }
            if (ds->hasNextNode()) {
                search_hit.setPrefix();
                search_hit.setMatchedDictSegment(ds);
            }
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

    std::shared_ptr<DictSegment> current = shared_from_this();
    for (const auto& rune : runes) {
        auto child = current->lookforSegment(rune.rune, true);
        if (!child) return;
        current = child;
    }
    current->node_state_ = 1;
}

std::shared_ptr<DictSegment> DictSegment::lookforSegment(int32_t keyChar, bool create) {
    std::shared_ptr<DictSegment> child = nullptr;

    if (store_size_ <= ARRAY_LENGTH_LIMIT) {
        for (size_t i = 0; i < store_size_ && i < children_array_.size(); i++) {
            if (children_array_[i]->key_char_ == keyChar) {
                child = children_array_[i];
                break;
            }
        }
        if (!child && create) {
            child = std::make_shared<DictSegment>(keyChar);
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
                    children_array_[i] = children_array_[i - 1];
                }
                children_array_[insertPos] = child;
                store_size_++;
            } else {
                for (size_t i = 0; i < store_size_; i++) {
                    children_map_[children_array_[i]->key_char_] = children_array_[i];
                }
                children_map_[keyChar] = child;
                store_size_++;
                children_array_.clear();
                children_array_.shrink_to_fit();
            }
        }
    } else {
        auto it = children_map_.find(keyChar);
        if (it != children_map_.end()) {
            child = it->second;
        } else if (create) {
            child = std::make_shared<DictSegment>(keyChar);
            children_map_[keyChar] = child;
            store_size_++;
        }
    }

    return child;
}

CL_NS_END2