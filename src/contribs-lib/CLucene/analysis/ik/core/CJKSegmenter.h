#ifndef CLUCENE_CJKSEGMENTER_H
#define CLUCENE_CJKSEGMENTER_H

#include <boost/pool/pool_alloc.hpp>
#include <list>
#include <memory>
#include <string>

#include "AnalyzeContext.h"
#include "CLucene/analysis/ik/dic/Dictionary.h"
#include "CharacterUtil.h"
#include "ISegmenter.h"

CL_NS_DEF2(analysis, ik)

class CJKSegmenter : public ISegmenter {
private:
    static const std::string SEGMENTER_NAME;
    std::list<Hit, boost::fast_pool_allocator<Hit>> tmp_hits_;

public:
    CJKSegmenter();

    void analyze(std::shared_ptr<AnalyzeContext> context) override;
    void reset() override;
};

CL_NS_END2
#endif //CLUCENE_CJKSEGMENTER_H
