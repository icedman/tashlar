#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <vector>

#include "editor.h"
#include "extension.h"
#include "view.h"

struct minimap_t : view_t {

    minimap_t();
    ~minimap_t();

    // view
    /*
    void layout(int x, int y, int width, int height) override;
    bool input(char ch, std::string keys) override;
    void calculate() override;
    */

    void update(int delta) override;
    void render() override;
    void applyTheme() override;

    editor_ptr editor;
    int currentLine;
};

void buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth);

#endif // MINIMAP_H
