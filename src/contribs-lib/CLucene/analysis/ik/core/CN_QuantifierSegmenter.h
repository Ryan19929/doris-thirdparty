#ifndef CLUCENE_CN_QUANTIFIERSEGMENTER_H
#define CLUCENE_CN_QUANTIFIERSEGMENTER_H

#include <set>
#include <memory>

#include "AnalyzeContext.h"
#include "ISegmenter.h"
#include "CLucene/analysis/ik/util/AllocatorManager.h"
CL_NS_DEF2(analysis, ik)

class CN_QuantifierSegmenter : public ISegmenter {
public:
    static const std::string SEGMENTER_NAME;
    static const std::u32string CHINESE_NUMBERS;
    static std::set<char32_t> chinese_number_chars_;

    CN_QuantifierSegmenter();
    ~CN_QuantifierSegmenter() override = default;

    void analyze(std::shared_ptr<AnalyzeContext> context) override;
    void reset() override;

private:
    void processCNumber(std::shared_ptr<AnalyzeContext> context);
    void processCount(std::shared_ptr<AnalyzeContext> context);
    bool needCountScan(std::shared_ptr<AnalyzeContext> context);
    void outputNumLexeme(std::shared_ptr<AnalyzeContext> context);

    int number_start_;
    int number_end_;
    IKVector<Hit> count_hits_;
};
CL_NS_END2
#endif //CLUCENE_CN_QUANTIFIERSEGMENTER_H
