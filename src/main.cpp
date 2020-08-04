#include <curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app.h"
#include "editor.h"
#include "explorer.h"
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

    search_t search;
    app_t app;
    app.configure(argc, argv);
    app.setupColors();

    // editor
    char* file = 0;
    if (argc > 1) {
        file = argv[argc - 1];
    } else {
        file = "./tests/test.cpp";
    }
    app.openEditor(file);
    app.explorer.setRootFromFile(file);

    app.applyTheme();

    while (true) {

        static struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        app.update(100);
        app.calculate();
        app.layout(0, 0, ws.ws_col, ws.ws_row);
        app.render();

        refresh();

        int ch = -1;
        std::string keySequence;
        while (true) {
            ch = readKey(keySequence);
            if (ch != -1) {
                break;
            }
        }

        if (keySequence == "ctrl+q") {
            break;
        }

        app_t::log("k: %d %s", ch, keySequence.c_str());
        statusbar_t::instance()->setStatus(keySequence);
        app.input(ch, keySequence);
    }

    endwin();
    return 0;
}
