#ifndef CLUCENE_DICTIONARY_H
#define CLUCENE_DICTIONARY_H

#include <CLucene.h>

#include <memory>
#include <string>
#include <mutex>
#include <vector>

#include "CLucene/LuceneThreads.h"
#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/analysis/ik/cfg/Configuration.h"
#include "CLucene/analysis/ik/core/CharacterUtil.h"
#include "DictSegment.h"
#include "Hit.hpp"

CL_NS_DEF2(analysis, ik)
CL_NS_USE(analysis)

class Dictionary {
private:
    static std::shared_ptr<Dictionary> singleton_;
    static std::once_flag init_flag_;
    // Dictionary segment mappings
    std::shared_ptr<DictSegment> main_dict_;
    std::shared_ptr<DictSegment> quantifier_dict_;
    std::shared_ptr<DictSegment> stop_words_;
    std::shared_ptr<Configuration> config_;
    bool load_ext_dict_;

    // Dictionary paths
    static const std::string PATH_DIC_MAIN;
    static const std::string PATH_DIC_QUANTIFIER;
    static const std::string PATH_DIC_STOP;

    explicit Dictionary(const Configuration& cfg, bool use_ext_dict = false);

    void loadMainDict();
    void loadStopWordDict();
    void loadQuantifierDict();

    void loadDictFile(std::shared_ptr<DictSegment> dict, const std::string& file_path,
                      bool critical, const std::string& dict_name);

    Dictionary(const Dictionary&) = delete;
    Dictionary& operator=(const Dictionary&) = delete;

public:
    static void initial(const Configuration& cfg, bool useExtDict = false) {
        getSingleton(cfg, useExtDict);
    }

    static std::shared_ptr<Dictionary> getSingleton() {
        if (!singleton_) {
            _CLTHROWA(CL_ERR_IllegalState, "Dictionary not initialized");
        }
        return singleton_;
    }

    static std::shared_ptr<Dictionary> getSingleton(const Configuration& cfg,
                                                    bool useExtDict = false) {
        std::call_once(init_flag_, [&]() {
            singleton_ = std::shared_ptr<Dictionary>(new Dictionary(cfg, useExtDict));
            singleton_->loadMainDict();
            singleton_->loadQuantifierDict();
            singleton_->loadStopWordDict();
        });
        return singleton_;
    }

    Configuration* getConfiguration() const { return config_.get(); }

    static void reload();

    Hit matchInMainDict(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset = 0,
                        size_t count = 0);
    Hit matchInQuantifierDict(const CharacterUtil::TypedRuneArray& typed_runes,
                              size_t unicode_offset = 0, size_t count = 0);
    void matchWithHit(const CharacterUtil::TypedRuneArray& typed_runes, int current_index,
                      Hit& hit);
    bool isStopWord(const CharacterUtil::TypedRuneArray& typed_runes, size_t unicode_offset = 0,
                    size_t count = 0);
    ~Dictionary() = default;
};

inline std::shared_ptr<Dictionary> Dictionary::singleton_ = nullptr;
inline std::once_flag Dictionary::init_flag_;

inline const std::string Dictionary::PATH_DIC_MAIN = "main.dic";
inline const std::string Dictionary::PATH_DIC_QUANTIFIER = "quantifier.dic";
inline const std::string Dictionary::PATH_DIC_STOP = "stopword.dic";

CL_NS_END2

#endif //CLUCENE_DICTIONARY_H
