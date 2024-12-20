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
    std::vector<std::unique_ptr<DictSegment>> children_array_;
    std::unordered_map<int32_t, std::unique_ptr<DictSegment>> children_map_;
    size_t store_size_ {0};
    int node_state_ {0};

    DictSegment* lookforSegment(int32_t key_char, bool create_if_missing);

public:
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
};

CL_NS_END2
#endif //CLUCENE_DICTSEGMENT_H
