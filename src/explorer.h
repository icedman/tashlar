#ifndef EXPLORER_H
#define EXPLORER_H

#include <string>
#include <vector>

#include "extension.h"

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

struct explorer_t {

    explorer_t();

    static explorer_t* instance();

    void update();
    void setRootFromFile(std::string path);

    void preloadFolders();
    void buildFileList(std::vector<struct fileitem_t*>& list, struct fileitem_t* files, int depth, bool deep = false);

    void print();
    
    std::vector<struct fileitem_t*> fileList();

    fileitem_t files;
    std::vector<struct fileitem_t*> renderList;
    std::vector<struct fileitem_t*> allFiles;
    bool allFilesLoaded;
    int loadDepth;

    int currentItem;
    bool regenerateList;
};

#endif // EXPLORER_H
