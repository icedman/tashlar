#ifndef EXPLORER_H
#define EXPLORER_H

#include <string>
#include <vector>

#include "extension.h"
#include "view.h"

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

struct explorer_t : view_t {

    explorer_t();

    static explorer_t* instance();

    // view
    void update(int delta) override;
    void render() override;
    void applyTheme() override;
    bool input(char ch, std::string keys) override;
    bool isVisible() override;
    void preLayout() override;
    void mouseDown(int x, int y, int button) override;

    void ensureVisibleCursor();

    void setRootFromFile(std::string path);

    void preloadFolders();
    void buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep = false);

    std::vector<struct fileitem_t*> fileList();

    fileitem_t files;
    std::vector<struct fileitem_t*> renderList;
    std::vector<struct fileitem_t*> allFiles;
    bool allFilesLoaded;
    int loadDepth;

    int currentItem;
};

#endif // EXPLORER_H
