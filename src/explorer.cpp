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
// #include "statusbar.h"
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
{
    preferredWidth = EXPLORER_WIDTH;

    canFocus = true;
    loadDepth = 0;
    allFilesLoaded = false;

    explorerInstance = this;
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
        allFiles.clear();
        renderList.clear();
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
}

void explorer_t::applyTheme()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    colorPrimary = pairForColor(comment.foreground.index, false);
    colorIndicator = pairForColor(app->tabActiveBorder, false);
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
    struct fileitem_t* item = renderList[currentItem];

    operation_e cmd = operationFromKeys(keys);

    switch (cmd) {
    case MOVE_CURSOR_RIGHT:
    case ENTER:
        if (item->isDirectory) {
            item->expanded = !item->expanded || cmd == MOVE_CURSOR_RIGHT;
            app->log("expand/collapse folder %s", item->fullPath.c_str());
            if (item->canLoadMore) {
                item->load();
                item->canLoadMore = false;
            }
            allFiles.clear();
            renderList.clear();
            return true;
        }
        if (cmd == ENTER) {
            app->log("open file %s", item->fullPath.c_str());
            app->openEditor(item->fullPath);
        }
        return true;
    case MOVE_CURSOR_LEFT:
        if (item->isDirectory && item->expanded) {
            item->expanded = false;
            allFiles.clear();
            renderList.clear();
            return true;
        }
        currentItem = parentItem(item, renderList)->lineNumber;
        break;
    case MOVE_CURSOR_UP:
        currentItem--;
        break;
    case MOVE_CURSOR_DOWN:
        currentItem++;
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
        // app->statusbar->setStatus(nextItem->fullPath, 3500);
        return true;
    }

    return false;
}
