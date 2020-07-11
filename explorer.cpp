#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "editor.h"
#include "app.h"
#include "explorer.h"
#include "util.h"

#define EXPLORER_WIDTH 15

filelist_t::filelist_t()
    : isDirectory(false)
    , canLoadMore(false)
{}

filelist_t::filelist_t(std::string p)
    : filelist_t()
{
    setPath(p);
}

void filelist_t::setPath(std::string p)
{
    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(p, delims);
    name = spath.back();
    path = p.erase(p.find(name));
}
    
void filelist_t::load(std::string p)
{
    setPath(p);
    for (const auto& filePath : enumerate_dir(path)) {
        app_t::instance()->log(filePath.c_str());

        std::string fullPath = path + "/";
        fullPath += filePath;
        std::shared_ptr<struct filelist_t> file = std::make_shared<struct filelist_t>(fullPath);
        files.emplace_back(file);
    }
}

void buildRenderList(std::vector<struct filelist_t*>& list, struct filelist_t *files)
{
    for(auto file : files->files) {
        if (file->name[0] == '.') {
            continue;
        }
        list.push_back(file.get());
    }
}
    
void explorer_t::render()
{
    renderList.clear();
    buildRenderList(renderList, &files);

    int y = 0;
    for(auto file : renderList) {
        wmove(win, y++, 0);
        renderLine(file->name.c_str()); 
       if (y > viewHeight) {
           break;
       }       
    }

    while (y < viewHeight) {
        wmove(win, y++, 0);
        wclrtoeol(win);
    }
}

void explorer_t::renderLine(const char* line)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
        if (idx + 1 >= viewWidth) {
            break;
        }
    }
}

void renderExplorer(struct explorer_t& explorer)
{
    if (!app_t::instance()->showSidebar) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor;
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t block = doc->block(cursor);

    //-----------------
    // calculate view
    //-----------------
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    explorer.viewX = 0;
    explorer.viewY = 0;
    explorer.viewWidth = EXPLORER_WIDTH;
    explorer.viewHeight = editor->viewHeight;

    if (!explorer.win) {
        explorer.win = newwin(explorer.viewHeight, explorer.viewWidth, 0, 0);
    }

    mvwin(explorer.win, explorer.viewY, explorer.viewX);
    wresize(explorer.win, explorer.viewHeight, explorer.viewWidth);

    wmove(explorer.win, 0, 0);

    explorer.render();
}