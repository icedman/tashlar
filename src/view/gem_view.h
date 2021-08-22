#ifndef GEM_VIEW_H
#define GEM_VIEW_H

#include "view.h"
#include "editor.h"
#include "editor_view.h"
#include "gutter_view.h"
#include "minimap_view.h"

struct gem_view_t : view_t {
    gem_view_t(editor_ptr editor);
    ~gem_view_t();

    void render() override;
    void update(int delta) override;

    editor_ptr editor;

    std::shared_ptr<editor_view_t> editorView;
    std::shared_ptr<gutter_view_t> gutterView;
    std::shared_ptr<minimap_view_t> minimapView;
};

typedef std::shared_ptr<gem_view_t> gem_view_ptr;
typedef std::vector<gem_view_ptr> gem_view_list;

void focusGem(editor_ptr editor, bool solo);
void gatherGems();
void removeGem(editor_ptr editor);

gem_view_list gemList();

#endif // GEM_VIEW_H