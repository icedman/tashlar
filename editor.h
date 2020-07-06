#ifndef EDITOR_H
#define EDITOR_H

#include <string>

#include "document.h"

// syntax highlighter
#include "grammar.h"
#include "parse.h"
#include "reader.h"
#include "theme.h"

#include "extension.h"

enum color_pair_e {
    NORMAL = 250,
    SELECTED
};

struct editor_t {
public:
    editor_t()
        : viewX(0)
        , viewY(0)
        , scrollX(0)
        , scrollY(0)
        , win(0)
    {
    }

    // view port
    int scrollX;
    int scrollY;
    int viewX;
    int viewY;
    int viewWidth;
    int viewHeight;
    int cursorScreenX;
    int cursorScreenY;

    struct document_t document;

    void highlightBlock(struct block_t& block);
    void renderBlock(struct block_t& block, int offsetX);
    void renderLine(const char* line, int offsetX = 0, struct block_t* block = 0);

    std::string clipBoard;
    language_info_ptr lang;
    theme_ptr theme;

    std::string status;

    WINDOW* win;
};

#endif // EDITOR_H