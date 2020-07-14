#ifndef POPUP_H
#define POPUP_H

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
        POPUP_COMMANDS,
        POPUP_FILES
    };
    
    popup_t()
        : window_t(true)
        , currentItem(-1)
    {
        focusable = true;
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void renderLine(const char* line, int offsetX, int &x);

    void hide();
    void search(std::string text);
    void files();
    void commands();

    void onInput();
    void onSubmit();

    std::string placeholder;
    std::string text;
    int currentItem;

    popup_e type;

    std::vector<struct item_t> items;
};
    
#endif // POPUP_H