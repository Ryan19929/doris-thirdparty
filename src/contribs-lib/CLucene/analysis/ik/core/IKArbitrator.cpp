#include "IKArbitrator.h"

CL_NS_USE2(analysis, ik)

void IKArbitrator::process(std::shared_ptr<AnalyzeContext> context, bool use_smart) {
    auto org_lexemes = context->getOrgLexemes();
    auto org_lexeme = org_lexemes->pollFirst();
    auto cross_path = std::make_unique<LexemePath>();
    while (org_lexeme) {
        if (!cross_path->addCrossLexeme(org_lexeme)) {
            // Find the next non-intersecting crossPath
            if (cross_path->size() == 1 || !use_smart) {
                // No ambiguity in crossPath or no ambiguity processing needed
                // Directly output current crossPath
                context->addLexemePath(std::move(cross_path));
            } else {
                // Process ambiguity for current crossPath
                auto headCell = cross_path->getHead();
                auto judgeResult = judge(headCell, cross_path->getPathLength());

                // Output ambiguity processing result
                context->addLexemePath(std::move(judgeResult));
            }

            // Add orgLexeme to new crossPath
            cross_path = std::make_unique<LexemePath>();
            cross_path->addCrossLexeme(org_lexeme);
        }
        org_lexeme = org_lexemes->pollFirst();
    }

    // Process the final path
    if (cross_path->size() == 1 || !use_smart) {
        // No ambiguity in crossPath or no ambiguity processing needed
        // Directly output current crossPath
        context->addLexemePath(std::move(cross_path));
    } else {
        // Process ambiguity for current crossPath
        auto headCell = cross_path->getHead();
        auto judgeResult = judge(headCell, cross_path->getPathLength());
        context->addLexemePath(std::move(judgeResult));
    }
}

std::unique_ptr<LexemePath> IKArbitrator::judge(std::shared_ptr<QuickSortSet::Cell> lexeme_cell, size_t full_text_length) {
    // Path options collection
    std::set<std::unique_ptr<LexemePath>, LexemePath::LexemePathPtrCompare> pathOptions;    // Candidate result path
    auto option = std::make_shared<LexemePath>();

    // Traverse crossPath once and return the stack of conflicting Lexemes
    auto lexemeStack = forwardPath(lexeme_cell, option);

    // Current lexeme chain is not ideal, add to path options collection
    pathOptions.insert(std::unique_ptr<LexemePath>(option->copy()));

    // Process ambiguous words if they exist
    while (!lexemeStack.empty()) {
        auto c = lexemeStack.top();
        lexemeStack.pop();
        // Rollback lexeme chain
        backPath(c->getLexeme(), option);
        // Start from ambiguous word position, recursively generate alternatives
        forwardPath(c, option);
        pathOptions.insert(std::unique_ptr<LexemePath>(option->copy()));
    }
    return std::unique_ptr<LexemePath>((*pathOptions.begin())->copy());
}

std::stack<std::shared_ptr<QuickSortSet::Cell>>IKArbitrator::forwardPath(
        std::shared_ptr<QuickSortSet::Cell> lexeme_cell, std::shared_ptr<LexemePath> path_option) {
    // Stack of conflicting Lexemes
    std::stack<std::shared_ptr<QuickSortSet::Cell>> conflictStack;
    auto current_cell = lexeme_cell;

    while (current_cell && current_cell->getLexeme() ) {
        if (!path_option->addNotCrossLexeme(current_cell->getLexeme())) {
            // Lexeme intersection, if addition fails, push to lexemeStack
            conflictStack.push(current_cell);
        }
        current_cell = current_cell->getNext();
    }
    return conflictStack;
}

void IKArbitrator::backPath(std::shared_ptr<Lexeme> lexeme, std::shared_ptr<LexemePath> option) {
    while(option->checkCross(lexeme)) {
        option->removeTail();
    }
}