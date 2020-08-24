#ifndef POPUP_H
#define POPUP_H

#include "cursor.h"
#include "view.h"

struct item_t {
    std::string name;
    std::string description;
    std::string fullPath;
    int score;
    int depth;
    std::string script;
};

struct popup_t : view_t {

    enum popup_e {
        POPUP_UNKNOWN,
        POPUP_SEARCH,
        POPUP_SEARCH_LINE,
        POPUP_COMMANDS,
        POPUP_FILES,
        POPUP_COMPLETION,
        POPUP_PROMPT
    };

    popup_t();

    static popup_t* instance();

    void update(int delta) override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
    void preLayout() override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseHover(int x, int y) override;

    void ensureVisibleCursor();

    void hide();
    void search(std::string text);
    void files();
    void commands();
    void completion();
    void showCompletion();
    void prompt(std::string placeholder);
    void onInput();
    void onSubmit();

    bool isCompletion();

    std::string placeholder;
    std::string text;
    int currentItem;

    popup_e type;
    struct cursor_t cursor;
    std::vector<struct item_t> items;
    std::vector<struct item_t> commandItems;

    int historyIndex;
    std::vector<std::string> commandHistory;
    std::vector<std::string> searchHistory;

    int request;
    int searchDirection;
};

#endif // POPUP_H
