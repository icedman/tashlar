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
#include "item.h"
#include "render.h"
#include "util.h"

#define PRELOAD_LOOP 8
#define MAX_PRELOAD_DEPTH 3
#define EXPLORER_WIDTH 20

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

static struct explorer_t* explorerInstance = 0;

struct explorer_t* explorer_t::instance()
{
    return explorerInstance;
}

explorer_t::explorer_t()
    : view_t("explorer")
    , currentItem(-1)
    , regenerateList(true)
{
    preferredWidth = EXPLORER_WIDTH;
    viewLayout = LAYOUT_VERTICAL;

    canFocus = true;
    loadDepth = 0;
    allFilesLoaded = false;

    explorerInstance = this;

    backgroundColor = 3;
}

void explorer_t::buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep)
{
    for (auto file : files->files) {
        file->depth = depth;
        file->lineNumber = list.size();
        list.push_back(file.get());
        if (file->expanded || deep) {
            buildFileList(list, file.get(), depth + 1, deep);
        }
    }

    /*
    while(views.size() > renderList.size()) {
        views.pop_back();
    }
    while(views.size() < renderList.size()) {
        item_view_t *item = new item_view_t();
        views.push_back((view_t*)item);
    }
    
    int idx = 0;
    for(auto i : renderList) {
        item_view_t *item = (item_view_t*)views[idx++];
        item->text = i->name;
        item->colorPrimary = colorPrimary;
        item->colorIndicator = colorIndicator;
    }
    */
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
    if (path.length() && path[path.length() - 1] != '/') {
        path = path.erase(path.find(file.name));
    }
    files.load(path);
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
        regenerateList = true;
        // render();
        // app_t::instance()->refresh();
    }
}

void explorer_t::update(int delta)
{
    /*
    if (!allFilesLoaded && app_t::instance()->isIdle() == 2) {
        preloadFolders();
    }
    */

    if (regenerateList) {
        regenerateList = false;
        allFiles.clear();
        renderList.clear();
        buildFileList(renderList, &files, 0);
    }
}

void explorer_t::preLayout()
{
    if (width == 0 || height == 0 || !isVisible())
        return;

    view_t::preLayout();

    preferredWidth = EXPLORER_WIDTH * render_t::instance()->fw;
    if (!render_t::instance()->isTerminal()) {
        preferredWidth += (padding * 2);
    }

    if (scrollbar->scrollTo >= 0 && scrollbar->scrollTo < scrollbar->maxScrollY && scrollbar->maxScrollY > 0) {
        scrollY = scrollbar->scrollTo;
        scrollbar->scrollTo = -1;
    }

    maxScrollY = (renderList.size() - rows + 1) * render_t::instance()->fh;
    scrollbar->setVisible(maxScrollY > rows && !render_t::instance()->isTerminal());
    scrollbar->scrollY = scrollY;
    scrollbar->maxScrollY = maxScrollY;
    scrollbar->colorPrimary = colorPrimary;

    scrollbar->thumbSize = rows / renderList.size();
    if (scrollbar->thumbSize < 2) {
        scrollbar->thumbSize = 2;
    }

    if (!scrollbar->isVisible() && !render_t::instance()->isTerminal()) {
        scrollY = 0;
    }
}

void explorer_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);

    for (auto view : views) {
        view->colorPrimary = colorPrimary;
        view->colorIndicator = colorIndicator;
    }
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

bool explorer_t::input(char ch, std::string keys)
{
    if (!isFocused()) {
        return false;
    }

    struct app_t* app = app_t::instance();
    struct fileitem_t* item = NULL;

    if (currentItem != -1) {
        item = renderList[currentItem];
    }

    operation_e cmd = operationFromKeys(keys);

    bool _scrollToCursor = true;

    switch (cmd) {
    case MOVE_CURSOR_RIGHT:
    case ENTER:
        if (item && item->isDirectory) {
            item->expanded = !item->expanded || cmd == MOVE_CURSOR_RIGHT;
            app->log("expand/collapse folder %s", item->fullPath.c_str());
            if (item->canLoadMore) {
                item->load();
                item->canLoadMore = false;
            }
            regenerateList = true;
            return true;
        }
        if (item && cmd == ENTER) {
            app->log("open file %s", item->fullPath.c_str());
            app->openEditor(item->fullPath);
        }
        return true;
    case MOVE_CURSOR_LEFT:
        if (item && item->isDirectory && item->expanded) {
            item->expanded = false;
            regenerateList = true;
            return true;
        }
        if (item) {
            currentItem = parentItem(item, renderList)->lineNumber;
        }
        break;
    case MOVE_CURSOR_UP:
        currentItem--;
        if (currentItem < 0) {
            currentItem = 0;
        }
        break;
    case MOVE_CURSOR_DOWN:
        currentItem++;
        if (currentItem >= renderList.size()) {
            currentItem = renderList.size() - 1;
        }
        break;
    case MOVE_CURSOR_START_OF_DOCUMENT:
        currentItem = 0;
        break;
    case MOVE_CURSOR_END_OF_DOCUMENT:
        currentItem = renderList.size() - 1;
        break;
    case MOVE_CURSOR_PREVIOUS_PAGE:
        currentItem -= height;
        break;
    case MOVE_CURSOR_NEXT_PAGE:
        currentItem += height;
        if (currentItem >= renderList.size()) {
            currentItem = renderList.size() - 1;
        }
        break;
    default:
        _scrollToCursor = false;
        break;
    }

    if (_scrollToCursor) {
        ensureVisibleCursor();
    }

    if (currentItem != -1) {
        struct fileitem_t* nextItem = renderList[currentItem];
        if (nextItem != item) {
            statusbar_t::instance()->setStatus(nextItem->fullPath, 3500);
            return true;
        }
    }

    return false;
}

void explorer_t::mouseDown(int x, int y, int button, int clicks)
{
    int prevItem = currentItem;
    int fh = render_t::instance()->fh;
    currentItem = ((y - this->y - padding) / fh) + (scrollY / fh);
    if (currentItem < 0 || currentItem >= renderList.size()) {
        currentItem = -1;
    }
    if (clicks > 0) {
        view_t::setFocus(this);
    }
    if (clicks > 1 || (clicks == 1 && prevItem == currentItem)) {
        input(0, "enter");
    }
}

void explorer_t::mouseHover(int x, int y)
{
    mouseDown(x, y, 1, 0);
}

bool explorer_t::isVisible()
{
    return visible && app_t::instance()->showSidebar;
}

void explorer_t::onFocusChanged(bool focused)
{
    if (focused && currentItem == -1 && renderList.size()) {
        currentItem = 0;
    }
}

void explorer_t::ensureVisibleCursor()
{
    int fh = render_t::instance()->fh;

    if (renderList.size()) {
        int current = currentItem;
        if (current < 0)
            current = 0;
        if (current >= renderList.size())
            current = renderList.size() - 1;
        int viewportHeight = rows - 0;

        while (renderList.size() > 4) {
            int blockVirtualLine = current;
            int blockScreenLine = blockVirtualLine - (scrollY / fh);

            app_t::instance()->log("b:%d v:%d s:%d %d", blockScreenLine, viewportHeight, scrollY / fh, current);

            if (blockScreenLine >= viewportHeight) {
                scrollY += fh;
            } else if (blockScreenLine <= 0) {
                scrollY -= fh;
            } else {
                break;
            }
        }
    } else {
        scrollY = 0;
    }
}

static void renderLine(const char* line, int& x, int width)
{
    char c;
    int idx = 0;
    while ((c = line[idx++])) {
        _addch(c);
        if (++x >= width - 1) {
            break;
        }
    }
}

void explorer_t::render()
{
    if (!isVisible()) {
        return;
    }

    // view_t::render();
    // return;

    app_t* app = app_t::instance();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    _move(0, 0);

    bool isHovered = view_t::currentHovered() == this;
    bool hasFocus = (isFocused() || isHovered) && !view_t::currentDragged();

    int idx = 0;
    int skip = (scrollY / render_t::instance()->fh);
    // app_t::log("??%d %d %d", scrollY, skip, render_t::instance()->fh);

    int y = 0;
    for (auto file : renderList) {
        if (skip-- > 0) {
            idx++;
            continue;
        }

        int pair = colorPrimary;
        _move(y, 0);
        _attron(_color_pair(pair));

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _reverse(true);
            } else {
                _bold(true);
            }
            for (int i = 0; i < cols; i++) {
                _addch(' ');
            }
        }

        if (file->fullPath == app_t::instance()->currentEditor->document.fullPath) {
            pair = colorIndicator;
        }

        int x = 0;
        _move(y++, x);
        _attron(_color_pair(pair));
        int indent = file->depth;
        for (int i = 0; i < indent; i++) {
            _addch(' ');
            x++;
        }
        if (file->isDirectory) {
            _attron(_color_pair(colorIndicator));

#ifdef ENABLE_UTF8
            _addwstr(file->expanded ? L"\u2191" : L"\u2192");
#else
            _addch(file->expanded ? '-' : '+');
#endif

            _attroff(_color_pair(colorIndicator));
            _attron(_color_pair(pair));
        } else {
            _addch(' ');
        }
        _addch(' ');
        x += 2;

        renderLine(file->name.c_str(), x, cols);

        if (hasFocus && currentItem == idx) {
            if (hasFocus) {
                pair = colorPrimary;
                _bold(true);
            } else {
                _bold(true);
            }
        }
        for (int i = 0; i < cols - x; i++) {
            _addch(' ');
        }

        _attroff(_color_pair(pair));
        _bold(false);
        _reverse(false);

        if (y >= rows + 1) {
            break;
        }

        idx++;
    }

    while (y < rows) {
        _move(y++, x);
        _clrtoeol(cols);
    }
}
