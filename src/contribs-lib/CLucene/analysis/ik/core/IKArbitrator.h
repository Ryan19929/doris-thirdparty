#ifndef CLUCENE_IKARBITRATOR_H
#define CLUCENE_IKARBITRATOR_H

#include <set>
#include <stack>
#include <memory>

#include "AnalyzeContext.h"
#include "CLucene/analysis/ik/util/AllocatorManager.h"
#include "LexemePath.h"
#include "QuickSortSet.h"

CL_NS_DEF2(analysis, ik)

class IKArbitrator {
public:
    IKArbitrator() = default;

    void process(std::shared_ptr<AnalyzeContext> context, bool use_smart);

private:
    std::unique_ptr<LexemePath> judge(QuickSortSet::Cell* lexeme_cell, size_t full_text_length);

    IKStack<QuickSortSet::Cell*> forwardPath(QuickSortSet::Cell* lexeme_cell, std::shared_ptr<LexemePath>  path_option);
    void forwardPath_void(QuickSortSet::Cell* lexeme_cell, std::shared_ptr<LexemePath> path_option);
    void backPath(const Lexeme& lexeme, std::shared_ptr<LexemePath> path_option);
};

CL_NS_END2
#endif //CLUCENE_IKARBITRATOR_H
