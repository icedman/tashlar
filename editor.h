#ifndef EDITOR_H
#define EDITOR_H

#include <curses.h>
#include <string>

#include "command.h"
#include "document.h"

// syntax highlighter
#include "grammar.h"
#include "parse.h"
#include "reader.h"
#include "theme.h"

#include "extension.h"

enum color_pair_e {
    NORMAL = 0,
    SELECTED
};

struct editor_t {
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
    void layoutBlock(struct block_t& block);
    void renderBlock(struct block_t& block, int offsetX, int offsetY);
    void renderLine(const char* line, int offsetX = 0, int offsetY = 0, struct block_t* block = 0, int relativeLine = 0);

    std::string clipBoard;
    language_info_ptr lang;
    theme_ptr theme;

    WINDOW* win;
};

struct statusbar_t;

#endif // EDITOR_H