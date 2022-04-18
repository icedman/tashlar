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
#include "util.h"

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

struct fileitem_t* parentItem(struct fileitem_t* item, std::vector<struct fileitem_t*>& list)
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

    // log("--------------------");
    // log("name: %s", name.c_str());
    // log("path: %s", path.c_str());
    // log("fullPath: %s", fullPath.c_str());
}

void fileitem_t::load(std::string p)
{
    if (p != "") {
        setPath(p);
    }

    // log("load %s", fullPath.c_str());
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
                // log("%s", suffix)
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

    // if (!files.size()) {
    //     explorer_t::instance()->setVisible(false);
    //     app_t::instance()->explorerScrollbar.setVisible(false);
    // }
}

static struct explorer_t* explorerInstance = 0;

struct explorer_t* explorer_t::instance()
{
    return explorerInstance;
}

explorer_t::explorer_t()
    : currentItem(-1)
    , regenerateList(true)
{
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

            // log("load more %d", item->depth);

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

void explorer_t::print()
{
    std::vector<struct fileitem_t*> files = fileList();

    for (auto file : files) {
        log(">%s\n", file->fullPath.c_str());
    }

}
