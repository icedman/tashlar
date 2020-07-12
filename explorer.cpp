#include <curses.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "statusbar.h"
#include "util.h"

#define EXPLORER_WIDTH 15

bool compareFile(std::shared_ptr<struct filelist_t> f1, std::shared_ptr<struct filelist_t> f2)
{
    if (f1->isDirectory && !f2->isDirectory) {
        return true;
    }
    if (!f1->isDirectory && f2->isDirectory) {
        return false;
    }
    return f1->name < f2->name;
}

filelist_t::filelist_t()
    : expanded(false)
    , isDirectory(false)
    , canLoadMore(false)
{
}

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

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }
            std::string filePath = ent->d_name;
            std::string fullPath = path + filePath;

            app_t::instance()->log(filePath.c_str());
            std::shared_ptr<struct filelist_t> file = std::make_shared<struct filelist_t>(fullPath);
            file->isDirectory = ent->d_type == DT_DIR;
            files.emplace_back(file);
        }
        closedir(dir);
    }

    sort(files.begin(), files.end(), compareFile);
}

void buildRenderList(std::vector<struct filelist_t*>& list, struct filelist_t* files)
{
    for (auto file : files->files) {
        list.push_back(file.get());
    }
}

void explorer_t::render()
{
    renderList.clear();
    buildRenderList(renderList, &files);

    int y = 0;
    for (auto file : renderList) {
        wmove(win, y++, 0);
        wclrtoeol(win);
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

void explorer_t::layout(int w, int h)
{
    if (!app_t::instance()->showSidebar) {
        viewWidth = 0;
        return;
    }
    viewX = 0;
    viewY = 0;
    viewWidth = EXPLORER_WIDTH;
    viewHeight = h - app_t::instance()->statusbar->viewHeight;
}

void explorer_t::renderCursor()
{
    wmove(win, 0, 0);
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

    if (!explorer.win) {
        explorer.win = newwin(explorer.viewHeight, explorer.viewWidth, 0, 0);
    }

    mvwin(explorer.win, explorer.viewY, explorer.viewX);
    wresize(explorer.win, explorer.viewHeight, explorer.viewWidth);

    wmove(explorer.win, 0, 0);

    explorer.render();
}
