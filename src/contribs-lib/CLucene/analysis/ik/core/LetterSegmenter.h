#ifndef CLUCENE_LETTERSEGMENTER_H
#define CLUCENE_LETTERSEGMENTER_H

#include <algorithm>
#include <vector>

#include "AnalyzeContext.h"
#include "ISegmenter.h"
CL_NS_DEF2(analysis, ik)

class LetterSegmenter : public ISegmenter {
public:
    static const std::string SEGMENTER_NAME;
    LetterSegmenter();
    ~LetterSegmenter() override = default;

    void analyze(shared_ptr<AnalyzeContext> context) override;
    void reset() override;

private:
    bool processEnglishLetter(shared_ptr<AnalyzeContext> context);
    bool processArabicLetter(shared_ptr<AnalyzeContext> context);
    bool processMixLetter(shared_ptr<AnalyzeContext> context);
    bool isLetterConnector(char input);
    bool isNumConnector(char input);

    int start_{-1};
    int end_{-1};
    int english_start_{-1};
    int english_end_{-1};
    int arabic_start_{-1};
    int arabic_end_{-1};

    std::vector<char> letter_connectors_;
    std::vector<char> num_connectors_;
};
CL_NS_END2

#endif //CLUCENE_LETTERSEGMENTER_H