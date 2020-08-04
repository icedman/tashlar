#include "block.h"
#include "document.h"
#include "app.h"

blockdata_t::blockdata_t()
    : dirty(true)
    , folded(false)
    , foldable(false)
    , foldedBy(0)
    , dots(0)
    , indent(0)
    , lastPrevBlockRule(0)
{
}

blockdata_t::~blockdata_t()
{
    if (dots) {
        free(dots);
    }
} 

block_t::block_t()
    : uid(0)
    , document(0)
    , lineNumber(0)
    , lineCount(0)
    , position(0)
    , screenLine(0)
    , _content(nullptr)
    , data(nullptr)
    , _length(0)
    , next(0)
    , previous(0)
{
}

block_t::~block_t()
{
}

std::string block_t::text()
{
    if (_content->dirty) {
        return _content->text;
    }

    std::string text;
    if (_content->file) {
        _content->file->seekg(_content->filePosition, _content->file->beg);
        size_t pos = _content->file->tellg();
        if (std::getline(*_content->file, text)) {
            _length = text.length() + 1;
            return text;
        }
    }

    return text;
}

void block_t::setText(std::string t)
{ 
    _content->text = t;  
    _length = t.length() + 1;
    
    _content->dirty = true;
    if (data) {
        data->dirty = true;
    }

    if (document) {
        document->dirty = true;
    }
}

bool block_t::isValid()
{
    if (document == 0 || _content == nullptr) {
        return false;
    }
    return true;
}

size_t block_t::length()
{
    return _length;
}

