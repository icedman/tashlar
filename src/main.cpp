#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "scripting.h"
#include "search.h"

#include "keyinput.h" // terminal input

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    initscr();
    raw();
    noecho();
    nodelay(stdscr, true);

    use_default_colors();
    start_color();

    curs_set(0);
    clear();

    scripting_t scripting;
    search_t search;
    app_t app;
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

        static struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        app.update(100);
        app.preLayout();
        app.layout(0, 0, ws.ws_col, ws.ws_row);
        app.preRender();
        app.render();

        refresh();

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

    endwin();
    return 0;
}
