#include "cursor.h"
#include "document.h"
#include "search.h"
#include "util.h"

#include <algorithm>

static size_t countIndentSize(std::string s)
{
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != ' ') {
            return i;
        }
    }
    return 0;
}

static int countToTabStop(struct cursor_t* cursor)
{
    int tab_size = 4;
    bool tab_to_spaces = true;
    if (!tab_to_spaces) {
        // cursor.insertText("\t");
        return 1;
    }

    int ts = tab_size;
    struct cursor_t cs = *cursor;
    cs.moveStartOfLine(true);
    size_t ws = countIndentSize(cs.selectedText() + "?");
    if (ws != 0 && ws == cursor->position()) {
        // magic! (align to tab)
        ts = (((ws / ts) + 1) * ts) - ws;
    }

    return ts;
}

static void cursorAtPreviousUnfoldedBlock(cursor_t& cursor, bool keepAnchor)
{
    block_ptr prev = cursor.block()->previous();
    while (prev) {
        cursor.setPosition(prev, cursor.cursor.position, keepAnchor);

        if (prev->data && prev->data->folded && !prev->data->foldable) {
            if (prev->previous()) {
                prev = prev->previous();
                continue;
            }
        }

        break;
    }
}

static void cursorAtNextUnfoldedBlock(cursor_t& cursor, bool keepAnchor)
{
    block_ptr next = cursor.block()->next();
    while (next) {
        cursor.setPosition(next, cursor.cursor.position % next->document->columns, keepAnchor);
        if (next->data && next->data->folded && !next->data->foldable) {
            if (next->next()) {
                next = next->next();
                continue;
            }
        }

        break;
    }
}

bool compareCursor(cursor_t a, cursor_t b)
{
    size_t aline = a.block()->lineNumber;
    size_t bline = b.block()->lineNumber;
    if (aline != bline) {
        return aline > bline;
    }
    return a.position() > b.position();
}

cursor_position_t::cursor_position_t()
    : block(nullptr)
{
}

cursor_t::cursor_t()
{
}

cursor_t::~cursor_t()
{
}

bool cursor_t::isNull()
{
    return block() == nullptr;
}

void cursor_t::setPosition(block_ptr b, size_t p, bool keepAnchor)
{
    if (b && p >= b->length()) {
        p = b->length() - 1;
    }
    cursor.block = b;
    cursor.position = p;
    if (!keepAnchor) {
        setAnchor(b, p);
    }
}

void cursor_t::setPosition(cursor_position_t pos, bool keepAnchor)
{
    setPosition(pos.block, pos.position, keepAnchor);
}

void cursor_t::setAnchor(block_ptr b, size_t p)
{
    anchor.block = b;
    anchor.position = p;
}

block_ptr cursor_t::block()
{
    return cursor.block;
}

size_t cursor_t::position()
{
    return cursor.position;
}

block_ptr cursor_t::anchorBlock()
{
    return anchor.block;
}

size_t cursor_t::anchorPosition()
{
    return anchor.position;
}

void cursor_t::clearSelection()
{
    anchor = cursor;
}

bool cursor_t::hasSelection()
{
    return (cursor.block.get() != anchor.block.get() || cursor.position != anchor.position);
}

bool cursor_t::isMultiBlockSelection()
{
    return (cursor.block.get() != anchor.block.get());
}

cursor_position_t cursor_t::selectionStart()
{
    if (cursor.block->lineNumber < anchor.block->lineNumber || (cursor.block->lineNumber == anchor.block->lineNumber && cursor.position < anchor.position)) {
        return cursor;
    }
    return anchor;
}

cursor_position_t cursor_t::selectionEnd()
{
    if (cursor.block->lineNumber > anchor.block->lineNumber || (cursor.block->lineNumber == anchor.block->lineNumber && cursor.position > anchor.position)) {
        return cursor;
    }
    return anchor;
}

std::string cursor_t::selectedText()
{
    std::string res;

    cursor_position_t start = selectionStart();
    cursor_position_t end = selectionEnd();

    block_ptr block = start.block;

    while (block) {
        if (res != "") {
            res += "\n";
        }
        std::string t = block->text();
        if (block == start.block) {
            t = t.substr(start.position);
        }
        if (block == end.block) {
            if (block == start.block) {
                // std::cout << start.position << " - " << end.position << std::endl;
                t = t.substr(0, end.position - start.position + 1);
            } else {
                t = t.substr(0, end.position + 1);
            }
        }
        res += t;

        if (block == end.block) {
            break;
        }
        block = block->next();
    }

    return res;
}

block_list cursor_t::selectedBlocks()
{
    block_list list;
    cursor_position_t start = selectionStart();
    cursor_position_t end = selectionEnd();
    block_ptr block = start.block;
    while (block) {
        list.push_back(block);
        if (block == end.block) {
            break;
        }
        block = block->next();
    }
    return list;
}

bool cursor_t::eraseSelection()
{
    if (!hasSelection()) {
        return false;
    }

    cursor_position_t start = selectionStart();
    cursor_position_t end = selectionEnd();

    std::string t = start.block->text().substr(0, start.position);
    if (end.position + 1 < end.block->length()) {
        t += end.block->text().substr(end.position + 1);
    }

    if (isMultiBlockSelection()) {
        int count = end.block->lineNumber - start.block->lineNumber;
        block()->document->removeBlockAtLine(start.block->lineNumber + 1, count);
    }

    start.block->setText(t);
    return true;
}

bool cursor_t::moveLeft(int count, bool keepAnchor)
{
    --count;

    if (cursor.position > 0) {
        cursor.position--;
    } else {
        if (block()->previous()) {
            cursorAtPreviousUnfoldedBlock(*this, keepAnchor);
            cursor.position = block()->length() - 1;
        } else {
            return false;
        }
    }

    if (count > 0) {
        return moveLeft(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveRight(int count, bool keepAnchor)
{
    --count;

    if (cursor.position + 1 < block()->length()) {
        cursor.position++;
    } else {
        if (block()->next()) {
            cursorAtNextUnfoldedBlock(*this, keepAnchor);
            cursor.position = 0;
        } else {
            return false;
        }
    }

    if (count > 0) {
        return moveRight(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveUp(int count, bool keepAnchor)
{
    --count;

    document_t* doc = block()->document;
    bool navigateWrappedLine = false;
    if (block()->lineCount > 1 && config_t::instance()->lineWrap && doc->columns) {
        int line = 1 + (cursor.position / doc->columns);
        if (line > 1) {
            if (cursor.position > doc->columns) {
                cursor.position -= doc->columns;
            } else {
                cursor.position = 0;
            }
            navigateWrappedLine = true;
        }
    }

    if (!navigateWrappedLine) {
        if (block()->previous()) {
            cursorAtPreviousUnfoldedBlock(*this, keepAnchor);
        } else {
            return false;
        }
    }

    if (count > 0) {
        return moveUp(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveDown(int count, bool keepAnchor)
{
    --count;

    document_t* doc = block()->document;
    bool navigateWrappedLine = false;
    if (block()->lineCount > 1 && config_t::instance()->lineWrap && doc->columns) {
        int line = 1 + (cursor.position / doc->columns);
        if (line < block()->lineCount) {
            cursor.position += doc->columns;
            if (cursor.position >= block()->length()) {
                cursor.position = block()->length() - 1;
            }
            navigateWrappedLine = true;
        }
    }

    if (!navigateWrappedLine) {
        if (block()->next()) {
            cursorAtNextUnfoldedBlock(*this, keepAnchor);
        } else {
            return false;
        }
    }

    if (count > 0) {
        return moveDown(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::movePreviousBlock(int count, bool keepAnchor)
{
    --count;
    if (block()->previous()) {
        cursorAtPreviousUnfoldedBlock(*this, keepAnchor);
    } else {
        return false;
    }

    if (count > 0) {
        return moveUp(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveNextBlock(int count, bool keepAnchor)
{
    --count;
    if (block()->next()) {
        cursorAtNextUnfoldedBlock(*this, keepAnchor);
    } else {
        return false;
    }

    if (count > 0) {
        return moveDown(count, keepAnchor);
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::movePreviousWord(bool keepAnchor)
{
    int startPosition = cursor.position;
    std::vector<search_result_t> search_results = search_t::instance()->findWords(block()->text());
    std::reverse(std::begin(search_results), std::end(search_results));
    bool found = false;
    for (auto i : search_results) {
        cursor.position = i.begin;
        if (i.begin + 1 < startPosition) {
            found = true;
            break;
        }
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveNextWord(bool keepAnchor)
{
    std::vector<search_result_t> search_results = search_t::instance()->findWords(block()->text());
    int startPosition = cursor.position;
    bool found = false;
    for (auto i : search_results) {
        cursor.position = i.begin;
        if (i.begin > startPosition) {
            // found = true;
            break;
        }
    }

    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::findWord(std::string t, int direction)
{
    struct cursor_t cur = *this;
    bool firstCursor = true;

    block_ptr block = cur.block();
    size_t prevPos = cur.position();

    while (block) {
        std::string text = block->text();
        std::vector<search_result_t> res = search_t::instance()->find(text, t);
        if (res.size()) {
            search_result_t found;
            if (firstCursor) {
                for (auto& r : res) {
                    if (r.end <= cur.position() || r.begin <= cur.position()) {
                        continue;
                    }
                    found = r;
                    break;
                }
            } else {
                found = res[0];
            }

            if (found.isValid()) {
                setPosition(cur.block(), found.end - 1);
                setAnchor(cur.block(), found.begin);
                return true;
            }
        }

        if (direction == 1) {
            if (!cur.movePreviousBlock()) {
                break;
            }
        } else {
            if (!cur.moveNextBlock()) {
                break;
            }
        }

        cur.moveStartOfLine();
        firstCursor = false;

        block = cur.block();
        prevPos = cur.position();
    }

    return false;
}

void cursor_t::selectWord()
{
    std::vector<search_result_t> search_results = search_t::instance()->findWords(block()->text());
    search_result_t word;
    bool found = false;
    for (auto i : search_results) {
        if (i.begin > cursor.position) {
            break;
        }
        word = i;
    }

    // cursor.position = word.begin;
    // anchor = cursor;
    // anchor.position = word.end - 1;

    anchor.position = word.begin;
    cursor = anchor;
    cursor.position = word.end - 1;
}

bool cursor_t::moveStartOfLine(bool keepAnchor)
{
    cursor.position = 0;
    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveEndOfLine(bool keepAnchor)
{
    cursor.position = cursor.block->length() - 1;
    if (!keepAnchor) {
        clearSelection();
    }

    return true;
}

bool cursor_t::moveStartOfDocument(bool keepAnchor)
{
    setPosition(cursor.block->document->firstBlock(), 0, keepAnchor);
    return true;
}

bool cursor_t::moveEndOfDocument(bool keepAnchor)
{
    setPosition(cursor.block->document->lastBlock(), 0, keepAnchor);
    moveEndOfLine(keepAnchor);
    return true;
}

bool cursor_t::insertText(std::string t)
{
    std::string blockText = block()->text();
    if (blockText.length() > 0) {
        blockText.insert(cursor.position, t);
    } else {
        blockText = t;
    }

    block()->setText(blockText);
    return true;
}

bool cursor_t::eraseText(int count)
{
    std::string blockText = block()->text();
    if (cursor.position < blockText.length()) {
        blockText.erase(cursor.position, count);
    }

    block()->setText(blockText);
    return true;
}

bool cursor_t::splitLine()
{
    std::string blockText = block()->text();
    std::string nextText;

    if (position() < blockText.length()) {
        nextText = blockText.substr(position());
    }

    blockText = blockText.substr(0, position());
    block()->setText(blockText);
    block()->document->addBlockAtLine(block()->lineNumber + 1)->setText(nextText);
    return true;
}

bool cursor_t::mergeNextLine()
{
    if (!block()->next()) {
        return false;
    }
    std::string blockText = block()->text();
    std::string nextText = block()->next()->text();
    block()->setText(blockText + nextText);
    block()->document->removeBlockAtLine(block()->lineNumber + 1);
    return true;
}

void cursor_t::print()
{
    // std::cout << block()->lineNumber << ":" << position() << std::endl;
}

void cursor_util::sortCursors(cursor_list& cursors)
{
    std::sort(cursors.begin(), cursors.end(), compareCursor);
}

void cursor_util::advanceBlockCursors(cursor_list& cursors, cursor_t cur, int advance)
{
    block_ptr block = cur.block();

    // cursors
    for (auto& cc : cursors) {
        if (cc.block() != block) {
            continue;
        }
        if (cc.position() <= cur.position()) {
            continue;
        }
        if (cc.position() + advance < 0) {
            cc.setPosition(block, 0, cc.hasSelection());
            continue;
        }
        cc.setPosition(block, cc.position() + advance, cc.hasSelection());
    }

    // anchors
    for (auto& cc : cursors) {
        if (!cc.hasSelection()) {
            continue;
        }
        if (cc.anchorBlock() != block) {
            continue;
        }
        if (cc.anchorPosition() <= cur.position()) {
            continue;
        }
        if (cc.anchorPosition() + advance < 0) {
            cc.setAnchor(block, 0);
            continue;
        }
        cc.setAnchor(block, cc.anchorPosition() + advance);
    }
}

static int _cursorIndent(cursor_t* cursor)
{
    std::string text = cursor->block()->text();
    int tab_size = 4;
    int currentIndent = countIndentSize(text + "?");
    int newIndent = ((currentIndent / tab_size) + 1) * tab_size;
    int inserted = newIndent - currentIndent;
    cursor_t cur = *cursor;

    std::string tab = "";
    for (int i = 0; i < inserted; i++) {
        tab += " ";
    }

    cur.cursor.position = 0;
    cur.insertText(tab);
    // cursor->cursor.position += inserted;
    // cursor->anchor.position += inserted;
    return inserted;
}

int cursor_t::indent()
{
    block_list blocks = selectedBlocks();

    if (blocks.size() > 1) {

        cursor_position_t posCur = selectionStart();
        cursor_position_t anchorCur = selectionEnd();

        int count = 0;
        int idx = 0;
        for (auto b : blocks) {
            cursor_t cur = *this;
            cur.setPosition(b, 0);
            count = _cursorIndent(&cur);
        }

        return count;
    }

    cursor_t cur = *this;
    cur.setPosition(cur.block(), 0);
    int count = _cursorIndent(&cur);
    return count;
}

static int _cursorUnindent(cursor_t* cursor)
{
    std::string text = cursor->block()->text();
    int tab_size = 4;
    int currentIndent = countIndentSize(text + "?");
    int newIndent = ((currentIndent / tab_size) - 1) * tab_size;
    int deleted = currentIndent - newIndent;
    cursor_t cur = *cursor;

    if (newIndent < 0) {
        deleted = deleted % tab_size;
    }

    if (deleted > 0) {
        cur.cursor.position = 0;
        cur.eraseText(deleted);
    }

    return deleted;
}

int cursor_t::unindent()
{
    block_list blocks = selectedBlocks();
    if (blocks.size() > 1) {

        cursor_position_t posCur = selectionStart();
        cursor_position_t anchorCur = selectionEnd();

        for (auto b : blocks) {
            struct cursor_t cur = *this;
            cur.setPosition(b, 0);

            bool updatePos = b == posCur.block;
            bool updateAnchor = b == anchorCur.block;
            int count = _cursorUnindent(&cur);

            if (updatePos) {
                log("unindent update pos %d", position());
                if (cursor.position - count < 0) {
                    cursor.position = 0;
                } else {
                    cursor.position -= count;
                }
            }
            if (updateAnchor) {
                log("unindent update anchor %d", anchorPosition());
                if (anchor.position - count < 0) {
                    anchor.position = 0;
                } else {
                    anchor.position -= count;
                }
            }
        }

        return 1;
    }

    cursor_t cur = *this;
    cur.setPosition(cur.block(), 0);
    int count = _cursorUnindent(&cur);
    return count;
}

static int _cursorToggleLineComment(cursor_t* cursor)
{
    /*

    // editor_ptr editor = app_t::instance()->currentEditor;
    editor_t *editor = cursor->block()->document->editor;
    if (!editor->highlighter.lang || !editor->highlighter.lang->lineComment.length()) {
        return 0;
    }

    std::string text = cursor->block()->text();

    // check existing comment
    int indent = countIndentSize(text);
    std::string trimmed(text.c_str() + indent);

    std::string singleLineComment = editor->highlighter.lang->lineComment;
    singleLineComment += " ";

    cursor->cursor.position = indent;

    if (trimmed.find(singleLineComment) == 0) {
        cursor->eraseText(singleLineComment.length());
        return -singleLineComment.length();
    }

    cursor->insertText(singleLineComment);
    return singleLineComment.length();
    */

    return 0;
}

int cursor_t::toggleLineComment()
{
    block_list blocks = selectedBlocks();

    if (blocks.size() > 1) {

        cursor_position_t posCur = selectionStart();
        cursor_position_t anchorCur = selectionEnd();

        int count = 0;
        int idx = 0;
        for (auto b : blocks) {
            cursor_t cur = *this;
            cur.setPosition(b, 0);
            count = _cursorToggleLineComment(&cur);
        }

        return count;
    }

    cursor_t cur = *this;
    cur.setPosition(cur.block(), 0);
    int count = _cursorToggleLineComment(&cur);
    return count;
}

bool cursor_t::isSelectionNormalized()
{
    if (!hasSelection()) {
        return false;
    }

    if (cursor.block != anchor.block) {
        return cursor.block->lineNumber < anchor.block->lineNumber;
    }

    return cursor.position < anchor.position;
}

void cursor_t::normalizeSelection(bool normalize)
{
    if (isSelectionNormalized() != normalize) {
        cursor_position_t tmp = cursor;
        cursor = anchor;
        anchor = tmp;
    }
}
