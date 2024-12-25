#include "IKArbitrator.h"

CL_NS_USE2(analysis, ik)

void IKArbitrator::process(AnalyzeContext& context, bool use_smart) {
    auto org_lexemes = context.getOrgLexemes();
    auto org_lexeme = org_lexemes->pollFirst();
    auto cross_path = std::make_unique<LexemePath>();

    auto process_path = [&](std::unique_ptr<LexemePath>& path) {
        if (path->size() == 1 || !use_smart) {
            context.addLexemePath(std::move(path));
        } else {
            context.addLexemePath(judge(path->getHead(), path->getPathLength()));
        }
    };

    while (org_lexeme) {
        if (!cross_path->addCrossLexeme(*org_lexeme)) {
            process_path(cross_path);
            cross_path = std::make_unique<LexemePath>();
            cross_path->addCrossLexeme(*org_lexeme);
        }
        org_lexeme = org_lexemes->pollFirst();
    }

    // Process the final path
    process_path(cross_path);
}

std::unique_ptr<LexemePath> IKArbitrator::judge(QuickSortSet::Cell* lexeme_cell,
                                                size_t full_text_length) {
    auto option = std::make_shared<LexemePath>();

    // Traverse crossPath once and return the stack of conflicting Lexemes
    auto lexemeStack = forwardPath(lexeme_cell, option);
    auto best_path = std::make_unique<LexemePath>(*option);

    // Process ambiguous words if they exist
    while (!lexemeStack.empty()) {
        auto c = lexemeStack.top();
        lexemeStack.pop();
        backPath(c->getLexeme(), option);
        forwardPath_void(c, option);
        if (*option < *best_path) {
            best_path = std::make_unique<LexemePath>(*option);
        }
    }
    return best_path;
}

void IKArbitrator::forwardPath_void(QuickSortSet::Cell* lexeme_cell,
                                    std::shared_ptr<LexemePath> path_option) {
    auto current_cell = lexeme_cell;

    while (current_cell) {
        path_option->addNotCrossLexeme(current_cell->getLexeme());
        current_cell = current_cell->getNext();
    }
}

IKStack<QuickSortSet::Cell*> IKArbitrator::forwardPath(QuickSortSet::Cell* lexeme_cell,
                                                       std::shared_ptr<LexemePath> path_option) {
    // Stack of conflicting Lexemes
    //    total_calls_++;
    IKStack<QuickSortSet::Cell*> conflictStack;

    auto current_cell = lexeme_cell;

    while (current_cell) {
        if (!path_option->addNotCrossLexeme(current_cell->getLexeme())) {
            conflictStack.push(current_cell);
        }
        current_cell = current_cell->getNext();
    }
    return conflictStack;
}

void IKArbitrator::backPath(const Lexeme& lexeme, std::shared_ptr<LexemePath> option) {
    while (option->checkCross(lexeme)) {
        option->removeTail();
    }
}