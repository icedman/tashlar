#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "render.h"
#include "scripting.h"
#include "search.h"

#include "keyinput.h" // terminal input

int main(int argc, char** argv)
{
    render_t renderer;
    scripting_t scripting;
    search_t search;
    app_t app;

    renderer.initialize();
    app.configure(argc, argv);
    app.setupColors();
    app.applyTheme();

    // editor
    char* file = 0;
    if (argc > 1) {
        file = argv[argc - 1];
    } else {
        file = "";
    }

    app.openEditor(file);
    app.explorer.setRootFromFile(file);

    scripting.initialize();
    std::string previousKeySequence;
    std::string expandedSequence;
    while (!app.end) {

        int delta = 100;
        renderer.update(delta);

        app.update(delta);
        app.preLayout();
        app.layout(0, 0, renderer.width, renderer.height);
        app.preRender();
        app.render();

        renderer.render();

        int ch = -1;
        std::string keySequence;
        while (true) {
            ch = readKey(keySequence);

            if (previousKeySequence.length() && keySequence.length()) {
                expandedSequence = previousKeySequence + "+" + keySequence;
            }

            if (ch != -1) {
                break;
            }

            app.update(100);
        }

        previousKeySequence = keySequence;
        if (expandedSequence.length()) {
            if (operationFromKeys(expandedSequence) != UNKNOWN) {
                keySequence = expandedSequence;
                previousKeySequence = "";
            }
            expandedSequence = "";
        }

        // app_t::log("k: %d %s", ch, keySequence.c_str());
        statusbar_t::instance()->setStatus(keySequence);
        app.input(ch, keySequence);
    }

    renderer.shutdown();
    return 0;
}
