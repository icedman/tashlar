#ifndef VIEW_CONTROLS_H
#define VIEW_CONTROLS_H

#include "view.h"

struct item_t {
    std::string name;
    std::string description;
    std::string fullPath;
    int score;
    int depth;
    std::string script;
};

struct list_view_t : view_t
{
	list_view_t();

    void update(int delta) override;
    void preLayout() override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    bool input(char ch, std::string keys) override;
    void applyTheme() override;
    // void mouseDown(int x, int y, int button, int clicks) override;
    // void mouseHover(int x, int y) override;

    void ensureVisibleCursor();

    int currentItem;

    void onInput() override;
    void onSubmit() override;

    std::vector<struct item_t> items;
    std::vector<struct item_t> commandItems;
};

struct text_view_t : view_t
{
    text_view_t();

    std::string text;

    void preLayout() override;
    void layout(int x, int y, int width, int height) override;
    void render() override;

    void applyTheme() override;
};

struct inputtext_view_t : view_t
{
	inputtext_view_t();

    std::string placeholder;
    std::string text;

    void preLayout() override;
    void layout(int x, int y, int width, int height) override;
    void render() override;
    bool input(char ch, std::string keys) override;

    void applyTheme() override;
};

#endif // VIEW_CONTROLS_H