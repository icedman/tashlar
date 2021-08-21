#include <stdio.h>
#include "app.h"
#include "search.h"
#include "statusbar.h"
#include "explorer.h"
#include "editor.h"
#include "operation.h"
#include "keyinput.h"
#include "util.h"

#include "view.h"
#include "app_view.h"
#include "editor_view.h"
#include "gem_view.h"
#include "explorer_view.h"
#include "statusbar_view.h"
#include "tabbar_view.h"
#include "popup_view.h"
#include "render.h"

int main(int argc, char **argv)
{
    const int delta = 100;

    app_t app;
    keybinding_t keybinding;
    explorer_t explorer;
    statusbar_t statusbar;
    search_t search;

    popup_root_view_t popups;

    render_t renderer;
    renderer.initialize();

    app.configure(argc, argv);
    app.setupColors();

    renderer.updateColors();
    
    std::string file = "./src/main.cpp";
    if (argc > 1) {
        file = argv[argc - 1];
    }

    app.openEditor(file);
    explorer_t::instance()->setRootFromFile(file);

    app_view_t root;
    explorer_view_t explr;
    statusbar_view_t sttbr;
    tabbar_view_t tbbr;

    view_t tabs;
    tabs.viewLayout = LAYOUT_VERTICAL;
    view_t content;
    content.viewLayout = LAYOUT_HORIZONTAL;

    tabs.addView(&tbbr);
    tabs.addView(&content);
    
    view_t::setMainContainer(&content);

    view_t main;
    main.viewLayout = LAYOUT_HORIZONTAL;
    main.addView(&explr);
    main.addView(&tabs);

    root.addView(&main);
    root.addView(&sttbr);
    root.applyTheme();

    popups.applyTheme();

    std::string previousKeySequence;
    std::string expandedSequence;
    while (!app.end) {
        renderer.update();

        root.update(delta);
        root.preLayout();
        root.layout(0, 0, renderer.width, renderer.height);
        root.preRender();
        root.render();

        popups.update(delta);
        popups.preLayout();
        popups.layout(0, 0, renderer.width, renderer.height);
        popups.preRender();
        popups.render();

        log("rows:%d", renderer.rows);

        renderer.render();

        if (!app.isFresh()) {
            continue;
        }

        int ch = -1;
        std::string keySequence;
        while (true) {
            renderer.input();
            ch = readKey(keySequence);

            if (previousKeySequence.length() && keySequence.length()) {
                expandedSequence = previousKeySequence + "+" + keySequence;
            }

            if (ch != -1) {
                break;
            }
        }

        if (ch == -1)
            continue;

        previousKeySequence = keySequence;
        if (expandedSequence.length()) {
            if (operationFromKeys(expandedSequence) != UNKNOWN) {
                keySequence = expandedSequence;
                previousKeySequence = "";
            }
            expandedSequence = "";
        }

        log("k: %d %s", ch, keySequence.c_str());

        // statusbar_t::instance()->setStatus(keySequence);
        // app.input(ch, keySequence);

        if (popups.input(ch, keySequence)) {
            continue;
        }

        root.input(ch, keySequence);
    }

    renderer.shutdown();
    return 0;
}
