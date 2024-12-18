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
    void printCollisionStats() const {
        if(!main_dict_) {
            std::cout << "字典树未初始化!" << std::endl;
            return;
        }

        DictSegment::CollisionStats stats;
        main_dict_->collectStats(stats);

        if(stats.map_nodes > 0) {
            stats.avg_load_factor /= stats.map_nodes;
        }

        std::cout << "\n字典树碰撞统计:\n";
        std::cout << "总节点数: " << stats.total_nodes << std::endl;
        std::cout << "数组存储节点数: " << stats.array_nodes << std::endl;
        std::cout << "哈希表存储节点数: " << stats.map_nodes << std::endl;
        std::cout << "平均负载因子: " << stats.avg_load_factor << std::endl;
        std::cout << "最大碰撞数: " << stats.max_collision << std::endl;

        std::cout << "\n碰撞分布:\n";
        std::cout << "碰撞数\t桶数量\t分布图\n";
        for(const auto& [collision, count] : stats.bucket_distribution) {
            std::cout << collision << "\t" << count << "\t";
            // 简单的柱状图
            const int scale = std::max(1ul, count / 100);  // 调整比例
            std::cout << std::string(count/scale, '*');
            std::cout << std::endl;
        }
    }
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
