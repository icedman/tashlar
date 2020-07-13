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
    {
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void update(int frames);
    
    void highlightBlock(struct block_t& block);
    void layoutBlock(struct block_t& block);
    void renderBlock(struct block_t& block, int offsetX, int offsetY);
    void renderLine(const char* line, int offsetX = 0, int offsetY = 0, struct block_t* block = 0, int relativeLine = 0);

    struct document_t document;
    language_info_ptr lang;
    theme_ptr theme;
};

struct editor_proxy_t : public window_t {
    editor_proxy_t()
        : window_t(true)
    {
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    bool isFocused() override;
};

typedef std::shared_ptr<struct editor_t> editor_ptr;

#endif // EDITOR_H