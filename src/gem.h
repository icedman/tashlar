#ifndef GEM_H
#define GEM_H

#include "editor.h"
#include "gutter.h"
#include "minimap.h"
#include "view.h"

#include <vector>

struct gem_t : view_t {
    gem_t();
    ~gem_t();

    gutter_t gutter;
    editor_ptr editor;
    minimap_t minimap;
    bool split;

    bool isVisible() override;
    bool input(char ch, std::string keys) override;
    void update(int delta) override;

    // view
    /*
    void layout(int x, int y, int width, int height) override;
    void render() override;
    void calculate() override;
    */
};

typedef std::shared_ptr<gem_t> gem_ptr;
typedef std::vector<gem_ptr> gem_list;

#endif // GEM_H