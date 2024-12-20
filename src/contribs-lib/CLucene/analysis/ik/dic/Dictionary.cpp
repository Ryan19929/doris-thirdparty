#include "Dictionary.h"

#include <fstream>

#include "CLucene/_ApiHeader.h"

CL_NS_DEF2(analysis, ik)

Dictionary::Dictionary(const Configuration& cfg, bool use_ext_dict)
        : config_(std::make_unique<Configuration>(cfg)),
          main_dict_(std::make_unique<DictSegment>(0)),
          quantifier_dict_(std::make_unique<DictSegment>(0)),
          stop_words_(std::make_unique<DictSegment>(0)),
          load_ext_dict_(use_ext_dict) {}

void Dictionary::loadDictFile(DictSegment * dict, const std::string& file_path,
                              bool critical, const std::string& dict_name) {
    std::ifstream in(file_path);
    if (!in.good()) {
        if (critical) {
            _CLTHROWA(CL_ERR_IO, (dict_name + " dictionary file not found: " + file_path).c_str());
        }
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        dict->fillSegment(line.c_str());
    }
}

void Dictionary::loadMainDict() {
    loadDictFile(main_dict_.get(), config_->getDictPath() + "/" + config_->getMainDictFile(), true,
                 "Main Dict");

    // Load extension dictionaries
    if (load_ext_dict_) {
        for (const auto& extDict : config_->getExtDictFiles()) {
            loadDictFile(main_dict_.get(), config_->getDictPath() + "/" + extDict, false, "Extra Dict");
        }
    }
}

void Dictionary::loadStopWordDict() {
    loadDictFile(stop_words_.get(), config_->getDictPath() + "/" + config_->getStopWordDictFile(), false,
                 "Stopword");
    // Load extension stop words dictionary
    if (load_ext_dict_) {
        for (const auto& extDict : config_->getExtStopWordDictFiles()) {
            loadDictFile(stop_words_.get(), config_->getDictPath() + "/" + extDict, false,
                         "Extra Stopword");
        }
    }
}

void Dictionary::loadQuantifierDict() {
    loadDictFile(quantifier_dict_.get(), config_->getDictPath() + "/" + config_->getQuantifierDictFile(),
                 true, "Quantifier");
}

void Dictionary::reload() {
    if (singleton_) {
        singleton_->loadMainDict();
        singleton_->loadStopWordDict();
        singleton_->loadQuantifierDict();
    }
}

Hit Dictionary::matchInMainDict(const CharacterUtil::TypedRuneArray& typed_runes,
                                size_t unicode_offset, size_t count) {
    Hit result;

    count = (count == 0 || unicode_offset + count > typed_runes.size())
                    ? typed_runes.size() - unicode_offset
                    : count;
    result = main_dict_.get()->match(typed_runes, unicode_offset, count);

    if (!result.isUnmatch()) {
        result.setByteBegin(typed_runes[unicode_offset].offset);
        result.setCharBegin(unicode_offset);
        result.setByteEnd(typed_runes[unicode_offset + count - 1].getNextBytePosition());
        result.setCharEnd(unicode_offset + count);
    }
    return result;
}

Hit Dictionary::matchInQuantifierDict(const CharacterUtil::TypedRuneArray& typed_runes,
                                      size_t unicode_offset, size_t count) {
    Hit result;

    count = (count == 0 || unicode_offset + count > typed_runes.size())
                    ? typed_runes.size() - unicode_offset
                    : count;
    result = quantifier_dict_.get()->match(typed_runes, unicode_offset, count);

    if (!result.isUnmatch()) {
        result.setByteBegin(typed_runes[unicode_offset].offset);
        result.setCharBegin(unicode_offset);
        result.setByteEnd(typed_runes[unicode_offset + count - 1].getNextBytePosition());
        result.setCharEnd(unicode_offset + count);
    }
    return result;
}

void Dictionary::matchWithHit(const CharacterUtil::TypedRuneArray& typed_runes, int current_index,
                              Hit& hit) {
    // 获取已匹配的DictSegment
    if (auto matchedSegment = hit.getMatchedDictSegment()) {
        matchedSegment->match(typed_runes, current_index, 1, hit);
        return;
    }
    hit.setUnmatch();
}

bool Dictionary::isStopWord(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset,
                            size_t count) {
    if (typed_runes.empty() || unicode_offset >= typed_runes.size()) {
        return false;
    }

    count = (count == 0 || unicode_offset + count > typed_runes.size())
                    ? typed_runes.size() - unicode_offset
                    : count;
    Hit result = stop_words_->match(typed_runes, unicode_offset, count);

    return result.isMatch();
}

CL_NS_END2