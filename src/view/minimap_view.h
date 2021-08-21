#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <vector>

#include "editor.h"
#include "extension.h"
#include "view.h"

struct gem_view_t;
struct minimap_view_t : view_t {

    minimap_view_t(editor_ptr editor);
    ~minimap_view_t();

    // view
    void update(int delta) override;
    void render() override;
    void applyTheme() override;
    bool isVisible() override;
    void preLayout() override;

    void scroll(int s) override;
    void mouseDown(int x, int y, int button, int clicks) override;
    void mouseUp(int x, int y, int button) override;
    void mouseDrag(int x, int y, bool within) override;
    void mouseHover(int x, int y) override;

    void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth);

    editor_ptr editor;
    int currentLine;
    size_t firstVisibleLine;
    size_t lastVisibleLine;

    int offsetY;
    int targetOffsetY;
};

#endif // MINIMAP_H
