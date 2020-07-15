#ifndef POPUP_H
#define POPUP_H

#include "cursor.h"
#include "window.h"

struct item_t {
    std::string name;
    std::string description;
    std::string fullPath;
    int score;
    int depth;
};

struct popup_t : public window_t {

    enum popup_e {
        POPUP_UNKNOWN,
        POPUP_SEARCH,
        POPUP_SEARCH_LINE,
        POPUP_COMMANDS,
        POPUP_FILES,
        POPUP_COMPLETION
    };

    popup_t()
        : window_t(true)
        , currentItem(-1)
        , request(0)
    {
        focusable = true;
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void renderLine(const char* line, int offsetX, int& x);
    void update(int frames) override;
    void hide();
    void search(std::string text);
    void files();
    void commands();
    void completion();
    void showCompletion();
    void onInput();
    void onSubmit();
    std::string placeholder;
    std::string text;
    int currentItem;

    popup_e type;
    struct cursor_t cursor;
    std::vector<struct item_t> items;

    int request;
};

#endif // POPUP_H
