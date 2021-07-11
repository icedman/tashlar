#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "keyinput.h"
#include "render.h"
#include "scripting.h"
#include "search.h"

int main(int argc, char** argv)
{
    keybinding_t keybinding;
    render_t renderer;
    scripting_t scripting;
    search_t search;
    app_t app;

    renderer.initialize();
    scripting.initialize();
    keybinding.initialize();

    app.configure(argc, argv);
    app.setupColors();
    app.applyTheme();

    // editor
    std::string file = "";
    if (argc > 1) {
        file = argv[argc - 1];
    }

    app.openEditor(file);
    app.explorer.setRootFromFile(file);

    std::string previousKeySequence;
    std::string expandedSequence;

    while (!app.end) {

        int delta = 100;
        renderer.update(delta);

        // log("h:%d", renderer.height);
        app.update(delta);
        app.preLayout();
        app.layout(0, 0, renderer.width, renderer.height);
        app.preRender();
        app.render();

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
