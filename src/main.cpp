#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "keyinput.h"
#include "render.h"
#include "scripting.h"
#include "search.h"

static bool minimal = false;

int editor(int argc, char** argv)
{
    const int delta = 100;

    keybinding_t keybinding;
    render_t renderer;
    scripting_t scripting;
    search_t search;
    app_t app;

    renderer.initialize();
    scripting.initialize();
    keybinding.initialize();

    app.configure(argc, argv);

    bool markup = app.markup != "";
    if (minimal || markup) {
        app.showStatusBar = false;
        app.showGutter = false;
        app.showTabbar = false;
        app.showSidebar = false;
        app.showMinimap = false;
        app.enablePopup = false;
    }

    app.setupView();
    app.setupColors();
    app.applyTheme();

    // editor
    std::string file = "";
    if (argc > 1) {
        file = argv[argc - 1];
    }

    app.openEditor(file);

    if (markup) {
        renderer.update(delta);
        app.update(delta);
        app.currentEditor->preLayout();
        app.currentEditor->layout(0,0,999999, 32);
        app.currentEditor->preRender();
        app.currentEditor->toMarkup();
        return 0;
    }

    app.explorer.setRootFromFile(file);

    std::string previousKeySequence;
    std::string expandedSequence;

    while (!app.end) {
        renderer.update(delta);

        // log("h:%d", renderer.height);
        app.update(delta);

        if (!minimal) {
            app.preLayout();
            app.layout(0, 0, renderer.width, renderer.height);
            app.preRender();
            app.render();
        } else {
            app.currentEditor->preLayout();
            app.currentEditor->layout(0,0,renderer.width, renderer.height);
            app.currentEditor->preRender();
            app.currentEditor->render();
        }

        renderer.render();

        if (app.refreshCount > 0) {
            app.refreshCount--;
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

            app.update(delta);
            if (app.refreshCount) {
                ch = -1;
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

        // log("k: %d %s", ch, keySequence.c_str());
        statusbar_t::instance()->setStatus(keySequence);
        app.input(ch, keySequence);
    }

    renderer.shutdown();
    return 0;
}

int main(int argc, char** argv)
{
    editor(argc, argv);
    return 0;
}
