#include "icons.h"
#include "app.h"
#include "util.h"

icons_t::icons_t()
{}

icons_t::~icons_t()
{}

std::string icons_t::iconForFile(std::string filename)
{
    std::set<char> delims = { '\\', '/' };
    std::vector<std::string> spath = split_path(filename, delims);

    std::string _suffix = spath.back();
    std::string cacheId = _suffix;
    // std::string cacheId = _suffix + color.name();

    static std::map<std::string, std::string> cache;
    auto it = cache.find(cacheId);
    if (it != cache.end()) {
        // std::cout << "cached icon.." << std::endl;
        return it->second;
    }

    Json::Value definitions = icons->definition["iconDefinitions"];

    std::string iconName;
    std::string fontCharacter = "x";
    std::string fontColor;

    Json::Value extensions = icons->definition["fileExtensions"];

    if (icons->definition.isMember(filename)) {
        iconName = icons->definition[filename].asString();
    }

    if (!iconName.length(), extensions.isMember(_suffix)) {
        iconName = extensions[_suffix].asString();
    }

    if (!iconName.length()) {
        Json::Value languageIds = icons->definition["languageIds"];

        std::string _fileName = "file." + _suffix;
        language_info_ptr lang = language_from_file(_fileName.c_str(), app_t::instance()->extensions);
        if (lang) {
            if (languageIds.isMember(lang->id)) {
                iconName = languageIds[lang->id].asString();
            }
        }

        if (!iconName.length()) {
            if (languageIds.isMember(_suffix)) {
                iconName = languageIds[_suffix].asString();
            }
        }
    }

    if (!iconName.length()) {
        std::string res = icons->definition["file"].asString();
        cache.emplace(cacheId, res);
        return res;
    }

    return "";
}

std::string icons_t::iconForFolder(std::string folder)
{
    return "";
}

