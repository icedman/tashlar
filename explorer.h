#ifndef EXPLORER_H
#define EXPLORER_H

#include <curses.h>
#include <string>
#include <vector>

#include "extension.h"
#include "window.h"

struct fileitem_t {

    fileitem_t();
    fileitem_t(std::string path);

    std::string name;
    std::string path;
    std::string fullPath;
    std::vector<std::shared_ptr<struct fileitem_t>> files;

    bool expanded;

    bool isDirectory;
    bool canLoadMore;

    int depth;
    int lineNumber;

    void setPath(std::string path);
    void load(std::string path = "");
};

struct explorer_t : public window_t {

    explorer_t()
        : window_t(true)
        , currentItem(-1)
    {
        focusable = true;
        loadDepth = 0;
        allFilesLoaded = false;
    }

    bool processCommand(command_e cmd, char ch) override;
    void layout(int w, int h) override;
    void render() override;
    void renderCursor() override;
    void renderLine(const char* line, int& offsetX);
    void setRootFromFile(std::string path);
    void update(int frames) override;

    void preloadFolders();

    std::vector<struct fileitem_t*> fileList();

    fileitem_t files;
    std::vector<struct fileitem_t*> renderList;
    std::vector<struct fileitem_t*> allFiles;
    bool allFilesLoaded;
    int loadDepth;

    int currentItem;
};

#endif // EXPLORER_H
