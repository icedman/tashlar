#ifndef EDITOR_H
#define EDITOR_H

#include <string>

#include "command.h"
#include "document.h"

// syntax highlighter
#include "grammar.h"
#include "parse.h"
#include "reader.h"
#include "theme.h"

#include "extension.h"
#include "window.h"

// 20K
// beyond this threshold, paste will use an additional file buffer
#define SIMPLE_PASTE_THRESHOLD 20000

enum color_pair_e {
    NORMAL = 0,
    SELECTED
};

struct editor_t : public window_t {
    editor_t()
        : window_t(true)
        , frames(0)
    {
    }

    bool processCommand(command_t cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void update(int frames) override;

    void highlightPreceedingBlocks(struct block_t& block);
    void highlightBlock(struct block_t& block);
    void layoutBlock(struct block_t& block);
    void renderBlock(struct block_t& block, int offsetX, int offsetY);
    void renderLine(const char* line, int offsetX = 0, int offsetY = 0, struct block_t* block = 0, int relativeLine = 0);

    void gatherBrackets(struct block_t& block, char* first, char* last);
    void matchBracketsUnderCursor();
    struct bracket_info_t bracketAtCursor(struct cursor_t& cursor);
    struct cursor_t cursorAtBracket(struct bracket_info_t bracket);
    struct cursor_t findLastOpenBracketCursor(struct block_t& block);
    struct cursor_t findBracketMatchCursor(struct bracket_info_t bracket, struct cursor_t cursor);

    void toggleFold(size_t line);

    struct document_t document;
    language_info_ptr lang;
    theme_ptr theme;

    struct bracket_info_t cursorBracket1;
    struct bracket_info_t cursorBracket2;

    int frames;
};

struct editor_proxy_t : public window_t {
    editor_proxy_t()
        : window_t(true)
    {
    }

    bool processCommand(command_t cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    bool isFocused() override;
    void update(int frames) override;
};

typedef std::shared_ptr<struct editor_t> editor_ptr;

#endif // EDITOR_H
