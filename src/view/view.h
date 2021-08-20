#ifndef VIEW_H
#define VIEW_H

#include <string>
#include <vector>

struct render_t;

enum layout_e {
    LAYOUT_UNKNOWN = 0,
    LAYOUT_VERTICAL,
    LAYOUT_HORIZONTAL,
    LAYOUT_STACK
};

struct view_t;
typedef std::vector<view_t*> view_list;

struct view_t {
    view_t(std::string name = "");
    ~view_t();

    std::string name;

    virtual void layout(int x, int y, int w, int h);
    virtual void update(int delta);
    virtual void render();
    virtual void preLayout();
    virtual void preRender();
    virtual bool input(char ch, std::string keys);
    virtual void applyTheme();
    virtual void scroll(int s);
    virtual void mouseDown(int x, int y, int button, int clicks);
    virtual void mouseUp(int x, int y, int button);
    virtual void mouseDrag(int x, int y, bool within);
    virtual void mouseHover(int x, int y);
    virtual void onFocusChanged(bool focused);
    virtual render_t* getRenderer();

    void vlayout(int x, int y, int w, int h);
    void hlayout(int x, int y, int w, int h);

    int padding;
    int contentWidth;
    int contentHeight;

    int width;
    int height;
    int x;
    int y;
    int rows;
    int cols;

    int flex;
    int preferredWidth;
    int preferredHeight;

    int scrollX;
    int scrollY;
    int maxScrollX;
    int maxScrollY;

    int colorPrimary;
    int colorSecondary;
    int colorSelected;
    int colorIndicator;

    static void setFocus(view_t* view);
    static view_t* currentFocus();
    static view_t* shiftFocus(int x, int y);

    static void setHovered(view_t* view);
    static view_t* currentHovered();

    static void setDragged(view_t* view);
    static view_t* currentDragged();

    view_t* viewFromPointer(int x, int y);

    bool isFocused();

    void setVisible(bool visible);
    virtual bool isVisible();

    bool visible;
    bool canFocus;
    bool floating;
    int backgroundColor;

    layout_e viewLayout;
    view_list views;

    void addView(view_t* view);

    static void setRoot(view_t *root);
    static view_t* getRoot();
    static view_t* getViewAt(int x, int y);
};

#endif // VIEW_H
