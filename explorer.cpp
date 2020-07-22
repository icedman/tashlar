#include <curses.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "app.h"
#include "editor.h"
#include "explorer.h"
#include "statusbar.h"
#include "util.h"

#define EXPLORER_WIDTH 20
#define PRELOAD_LOOP 8
#define MAX_PRELOAD_DEPTH 3

static bool compareFile(std::shared_ptr<struct fileitem_t> f1, std::shared_ptr<struct fileitem_t> f2)
{
    if (f1->isDirectory && !f2->isDirectory) {
        return true;
    }
    if (!f1->isDirectory && f2->isDirectory) {
        return false;
    }
    return f1->name < f2->name;
}

fileitem_t::fileitem_t()
    : depth(0)
    , expanded(false)
    , isDirectory(false)
    , canLoadMore(false)
{
}

fileitem_t::fileitem_t(std::string p)
    : fileitem_t()
{
    setPath(p);
}

void fileitem_t::setPath(std::string p)
{
    path = p;
    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(p, delims);
    name = spath.back();

    char* cpath = (char*)malloc(path.length() + 1 * sizeof(char));
    strcpy(cpath, path.c_str());
    expand_path((char**)(&cpath));
    fullPath = std::string(cpath);
    free(cpath);

    // app_t::instance()->log("--------------------");
    // app_t::instance()->log("name: %s", name.c_str());
    // app_t::instance()->log("path: %s", path.c_str());
    // app_t::instance()->log("fullPath: %s", fullPath.c_str());
}

void fileitem_t::load(std::string p)
{
    if (p != "") {
        setPath(p);
    }

    // app_t::instance()->log("load %s", fullPath.c_str());
    std::vector<std::string>& excludeFiles = app_t::instance()->excludeFiles;
    std::vector<std::string>& excludeFolders = app_t::instance()->excludeFolders;

    int safety = 0;
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(fullPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }

            std::string filePath = ent->d_name;
            std::string fullPath = path + "/" + filePath;

            size_t pos = fullPath.find("//");
            if (pos != std::string::npos) {
                fullPath.replace(fullPath.begin() + pos, fullPath.begin() + pos + 2, "/");
            }
            std::shared_ptr<struct fileitem_t> file = std::make_shared<struct fileitem_t>(fullPath);
            file->isDirectory = ent->d_type == DT_DIR;
            file->canLoadMore = file->isDirectory;

            bool exclude = false;
            if (file->isDirectory) {
                for (auto pat : excludeFolders) {
                    if (filePath == pat) {
                        exclude = true;
                        break;
                    }
                }
            } else {
                std::set<char> delims = { '.' };
                std::vector<std::string> spath = split_path(filePath, delims);
                std::string suffix = "*." + spath.back();
                // app_t::instance()->log("%s", suffix)
                for (auto pat : excludeFiles) {
                    if (suffix == pat) {
                        exclude = true;
                        break;
                    }
                }
            }

            if (exclude) {
                continue;
            }
            files.emplace_back(file);
        }
        closedir(dir);
    }

    sort(files.begin(), files.end(), compareFile);
}

static void buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep = false)
{
    for (auto file : files->files) {
        file->depth = depth;
        file->lineNumber = list.size();
        list.push_back(file.get());
        if (file->expanded || deep) {
            buildFileList(list, file.get(), depth + 1, deep);
        }
    }
}

std::vector<struct fileitem_t*> explorer_t::fileList()
{
    if (!allFiles.size()) {
        buildFileList(allFiles, &files, 0, true);
    }
    return allFiles;
}

void explorer_t::setRootFromFile(std::string path)
{
    fileitem_t file;
    file.setPath(path);
    path = path.erase(path.find(file.name));
    files.load(path);
}

void explorer_t::render()
{
    if (!app_t::instance()->showSidebar) {
        return;
    }

    struct editor_t* editor = app_t::instance()->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    if (!win) {
        win = newwin(viewHeight, viewWidth, 0, 0);
    }

    mvwin(win, viewY, viewX);
    wresize(win, viewHeight, viewWidth);

    wmove(win, 0, 0);

    if (renderList.size() == 0) {
        buildFileList(renderList, &files, 0);
    }

    bool hasFocus = isFocused();
    if (currentItem == -1) {
        currentItem = 0;
    }

    // scroll to cursor
    // TODO: use math not loops
    while (true) {
        int blockVirtualLine = currentItem;
        int blockScreenLine = blockVirtualLine - scrollY;

        // app_t::instance()->log(">%d %d scroll:%d items:%d h:%d", blockScreenLine, currentItem, scrollY, renderList.size(), viewHeight);

        if (blockScreenLine + 1 >= viewHeight) {
            scrollY++;
        } else if (blockScreenLine < 0) {
            scrollY--;
        } else {
            break;
        }
    }

    int idx = 0;
    int skip = scrollY;
    int y = 0;
    for (auto file : renderList) {
        if (skip-- > 0) {
            idx++;
            continue;
        }

        int pair = colorPair;
        wmove(win, y, 0);
        wclrtoeol(win);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                // pair = colorPairSelected;
                wattron(win, A_REVERSE);
            } else {
                wattron(win, A_BOLD);
            }
            for (int i = 0; i < viewWidth; i++) {
                waddch(win, ' ');
            }
        }

        int x = 0;
        wmove(win, y++, 0);
        wattron(win, COLOR_PAIR(pair));
        int indent = file->depth;
        for (int i = 0; i < indent; i++) {
            waddch(win, ' ');
            x++;
        }
        if (file->isDirectory) {
            // waddch(win, file->expanded ? '-' : '+');
            // waddwstr(win, L"\u276F"); // arrow
            // waddwstr(win, L"\u2716"); // close
            // waddwstr(win, file->expanded ? L"\u2303" : L"\u203A");
            // waddwstr(win, file->expanded ? L"\u25B4" : L"\u25B8");
            // waddwstr(win, file->expanded ? L"\u25B2" : L"\u25B6");
            wattron(win, COLOR_PAIR(colorPairIndicator));
            // waddwstr(win, file->expanded ? L"\u2191" : L"\u2192");
            wattroff(win, COLOR_PAIR(colorPairIndicator));
            wattron(win, COLOR_PAIR(pair));
        } else {
            // waddwstr(win, L"\u1F4C4");
            waddch(win, ' ');
        }
        waddch(win, ' ');
        x += 2;

        renderLine(file->name.c_str(), x);

        //for (int i = 0; i < viewWidth - x; i++) {
        //    waddch(win, ' ');
        //}
        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                // pair = colorPairSelected;
                wattron(win, A_REVERSE);
            } else {
                wattron(win, A_BOLD);
            }
            for (int i = 0; i < viewWidth - x; i++) {
                waddch(win, ' ');
            }
        }

        wattroff(win, COLOR_PAIR(pair));
        wattroff(win, A_REVERSE);
        wattroff(win, A_BOLD);

        if (y >= viewHeight) {
            break;
        }

        idx++;
    }

    while (y < viewHeight) {
        wmove(win, y++, 0);
        // wclrtoeol(win);
        for (int i = 0; i < viewWidth; i++) {
            waddch(win, ' ');
        }
    }

    wrefresh(win);
}

void explorer_t::preloadFolders()
{
    if (allFilesLoaded) {
        return;
    }

    if (!allFiles.size()) {
        buildFileList(allFiles, &files, 0, true);
    }

    int loaded = 0;
    for (auto item : allFiles) {
        if (item->isDirectory && item->canLoadMore && item->depth == loadDepth) {
            item->load();

            // app_t::instance()->log("load more %d", item->depth);

            item->canLoadMore = false;
            if (loaded++ >= PRELOAD_LOOP) {
                break;
            }
        }
    }

    if (loaded == 0) {
        loadDepth++;
    }

    allFilesLoaded = (loaded == 0 && loadDepth >= MAX_PRELOAD_DEPTH);
    if (loaded > 0) {
        allFiles.clear();
        renderList.clear();
        render();
        app_t::instance()->refresh();
    }
}

void explorer_t::update(int frames)
{
    if (!allFilesLoaded && app_t::instance()->isIdle() == 2) {
        preloadFolders();
    }
}

void explorer_t::renderLine(const char* line, int& x)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        waddch(win, c);
        if (++x >= viewWidth - 1) {
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
    viewHeight = h;

    if (app_t::instance()->showStatusBar) {
        int statusbarHeight = app_t::instance()->statusbar->viewHeight;
        viewHeight -= statusbarHeight;
    }
}

void explorer_t::renderCursor()
{
    wmove(win, 0, 0);
}

static struct fileitem_t* parentItem(struct fileitem_t* item, std::vector<struct fileitem_t*>& list)
{
    int depth = item->depth;
    int i = item->lineNumber;
    while (i-- > 0) {
        struct fileitem_t* res = list[i];
        if (!res) {
            break;
        }
        if (res->depth < depth) {
            return res;
        }
    }
    return item;
}

bool explorer_t::processCommand(command_t cmdt, char ch)
{
    // proceed only if got focus
    if (!isFocused()) {
        return false;
    }

    command_e cmd = cmdt.cmd;
    struct app_t* app = app_t::instance();
    struct fileitem_t* item = renderList[currentItem];

    switch (cmd) {
    case CMD_MOVE_CURSOR_RIGHT:
    case CMD_ENTER:
        if (item->isDirectory) {
            item->expanded = !item->expanded || cmd == CMD_MOVE_CURSOR_RIGHT;
            app->log("expand/collapse folder %s", item->fullPath.c_str());
            if (item->canLoadMore) {
                item->load();
                item->canLoadMore = false;
            }
            allFiles.clear();
            renderList.clear();
            return true;
        }
        if (cmd == CMD_ENTER) {
            app->log("open file %s", item->fullPath.c_str());
            app->openEditor(item->fullPath);
            app->refresh();
        }
        return true;
    case CMD_MOVE_CURSOR_LEFT:
        if (item->isDirectory && item->expanded) {
            item->expanded = false;
            allFiles.clear();
            renderList.clear();
            return true;
        }
        currentItem = parentItem(item, renderList)->lineNumber;
        break;
    case CMD_MOVE_CURSOR_UP:
        currentItem--;
        break;
    case CMD_MOVE_CURSOR_DOWN:
        currentItem++;
        break;
    case CMD_MOVE_CURSOR_START_OF_DOCUMENT:
        currentItem = 0;
        break;
    case CMD_MOVE_CURSOR_END_OF_DOCUMENT:
        currentItem = renderList.size() - 1;
        break;
    case CMD_MOVE_CURSOR_PREVIOUS_PAGE:
        currentItem -= viewHeight;
        break;
    case CMD_MOVE_CURSOR_NEXT_PAGE:
        currentItem += viewHeight;
        break;
    default:
        break;
    }

    // validate
    if (currentItem < 0) {
        currentItem = 0;
    }
    if (currentItem >= renderList.size()) {
        currentItem = renderList.size() - 1;
    }

    struct fileitem_t* nextItem = renderList[currentItem];
    if (nextItem != item) {
        app->statusbar->setStatus(nextItem->fullPath, 3500);
        return true;
    }

    return false;
}
