#include "highlighter.h"
#include "document.h"
#include "parse.h"
#include "util.h"
#include "editor.h"
#include "indexer.h"
#include "render.h"
#include "app.h"

#include <cstring>
#include <unistd.h>

#define LINE_LENGTH_LIMIT 500

highlighter_t::highlighter_t()
    : threadId(0)
    , editor(0)
{
}

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos)
{
    span_info_t res;
    res.bold = false;
    res.italic = false;
    res.length = 0;
    for (auto span : blockData->spans) {
        if (span.length == 0) {
            continue;
        }
        if (pos >= span.start && pos < span.start + span.length) {
            res = span;
            if (res.state == BLOCK_STATE_UNKNOWN) {
                // log("unknown >>%d", span.start);
            }
            return res;
        }
    }

    return res;
}

void highlighter_t::highlightBlocks(block_ptr block, int count)
{
    while(block && count-- > 0) {
        highlightBlock(block);
        block = block->next();
    }
}

void highlighter_t::highlightBlock(block_ptr block)
{
    if (!block)
        return;

    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    if (!block->data->dirty) {
        return;
    }

    if (!lang) {
        return;
    }

    // log("hl %d", block->lineNumber);

    struct blockdata_t* blockData = block->data.get();

    bool previousState = blockData->state;
    block_state_e previousBlockState = BLOCK_STATE_UNKNOWN;

    block_ptr prevBlock = block->previous();
    struct blockdata_t* prevBlockData = NULL;
    if (prevBlock) {
        prevBlockData = prevBlock->data.get();
    }

    std::string text = block->text();
    std::string str = text;
    str += "\n";

    //--------------
    // for minimap line cache
    //--------------
    for (int i = 0; i < 4; i++) {
        int ln = block->lineNumber - 4 + i;
        if (ln < 0) {
            ln = 0;
        }
        block_ptr pb = block->document->blockAtLine(ln);
        if (pb->data && pb->data->dots) {
            free(pb->data->dots);
            pb->data->dots = 0;
        }
    }

    //--------------
    // call the grammar parser
    //--------------
    bool firstLine = true;
    parse::stack_ptr parser_state = NULL;
    std::map<size_t, scope::scope_t> scopes;
    // blockData->scopes.clear();
    blockData->spans.clear();

    const char* first = str.c_str();
    const char* last = first + text.length() + 1;

    if (prevBlockData) {
        previousBlockState = prevBlockData->state;
        parser_state = prevBlockData->parser_state;
        if (parser_state && parser_state->rule) {
            blockData->lastPrevBlockRule = parser_state->rule->rule_id;
        }
        firstLine = !(parser_state != NULL);
    }

    if (!parser_state) {
        parser_state = lang->grammar->seed();
        firstLine = true;
    }

    if (text.length() > LINE_LENGTH_LIMIT) {
        // too long to parse
        blockData->dirty = false;
        return;
    } else {
        parser_state = parse::parse(first, last, parser_state, scopes, firstLine);
    }

    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    size_t n = 0;
    size_t l = block->length();
    while (it != scopes.end()) {
        n = it->first;
        scope::scope_t scope = it->second;
        std::string scopeName(scope);
        style_t s = theme->styles_for_scope(scopeName);

        block_state_e state;
        if (scopeName.find("comment") != std::string::npos) {
            state = BLOCK_STATE_COMMENT;
        } else if (scopeName.find("string") != std::string::npos) {
            state = BLOCK_STATE_STRING;
        } else {
            state = BLOCK_STATE_UNKNOWN;
        }

        style_t style = theme->styles_for_scope(scopeName);
        span_info_t span = {
                    .start = n,
                    .length = l - n,
                    .colorIndex = style.foreground.index,
                    .bold = style.bold == bool_true,
                    .italic = style.italic == bool_true,
                    .state = state,
                    .scope = scopeName
                };

        if (blockData->spans.size() > 0) {
            span_info_t &prevSpan = blockData->spans.front();
            prevSpan.length = n - prevSpan.start;
        }
        blockData->spans.insert(blockData->spans.begin(), 1, span);
        // blockData->spans.push_back(span);
        it++;
    }

    //----------------------
    // langauge config
    //----------------------

    //----------------------
    // find block comments
    //----------------------
    if (lang->blockCommentStart.length()) {
        size_t beginComment = text.find(lang->blockCommentStart);
        size_t endComment = text.find(lang->blockCommentEnd);
        style_t s = theme->styles_for_scope("comment");

        if (endComment == std::string::npos && (beginComment != std::string::npos || previousBlockState == BLOCK_STATE_COMMENT)) {
            blockData->state = BLOCK_STATE_COMMENT;
            int b = beginComment != std::string::npos ? beginComment : 0;
            int e = endComment != std::string::npos ? endComment : (last - first);

            span_info_t span = {
                .start = b,
                .length = e - b,
                .colorIndex = s.foreground.index,
                .bold = s.bold == bool_true,
                .italic = s.italic == bool_true,
                .state = BLOCK_STATE_COMMENT,
                .scope = "comment"
            };
            blockData->spans.insert(blockData->spans.begin(), 1, span);

        } else if (beginComment != std::string::npos && endComment != std::string::npos) {
            blockData->state = BLOCK_STATE_UNKNOWN;
            int b = beginComment;
            int e = endComment + lang->blockCommentEnd.length();
            
            span_info_t span = {
                .start = b,
                .length = e - b,
                .colorIndex = s.foreground.index,
                .bold = s.bold == bool_true,
                .italic = s.italic == bool_true,
                .state = BLOCK_STATE_COMMENT,
                .scope = "comment"
            };
            blockData->spans.insert(blockData->spans.begin(), 1, span);
        } else {
            blockData->state = BLOCK_STATE_UNKNOWN;
            if (endComment != std::string::npos && previousBlockState == BLOCK_STATE_COMMENT) {
                // setFormatFromStyle(0, endComment + lang->blockCommentEnd.length(), s, first, blockData, "comment");
                span_info_t span = {
                    .start = 0,
                    .length = endComment + lang->blockCommentEnd.length(),
                    .colorIndex = s.foreground.index,
                    .bold = s.bold == bool_true,
                    .italic = s.italic == bool_true,
                    .state = BLOCK_STATE_UNKNOWN,
                    .scope = "comment"
                };
                blockData->spans.insert(blockData->spans.begin(), 1, span);
            }
        }
    }

    if (lang->lineComment.length()) {
        // comment out until the end
    }

    //----------------------
    // reset brackets
    //----------------------
    blockData->brackets.clear();
    blockData->foldable = false;
    blockData->foldingBrackets.clear();
    blockData->ifElseHack = false;
    gatherBrackets(block, (char*)first, (char*)last);

    blockData->parser_state = parser_state;
    blockData->dirty = false;
    blockData->lastRule = parser_state->rule->rule_id;
    if (blockData->state == BLOCK_STATE_COMMENT) {
        blockData->lastRule = BLOCK_STATE_COMMENT;
    }

    //----------------------
    // mark next block for highlight
    // .. if necessary
    //----------------------
    if (blockData->lastRule) {
        block_ptr next = block->next();
        if (next && next->isValid()) {
            struct blockdata_t* nextBlockData = next->data.get();
            if (nextBlockData && blockData->lastRule != nextBlockData->lastPrevBlockRule) {
                nextBlockData->dirty = true;
            }

            if (nextBlockData && (blockData->lastRule == BLOCK_STATE_COMMENT || nextBlockData->lastRule == BLOCK_STATE_COMMENT)) {
                nextBlockData->dirty = true;
            }
        }
    }

    if (editor && editor->indexer) {
        editor->indexer->updateBlock(block);
    }
}

void highlighter_t::gatherBrackets(block_ptr block, char* first, char* last)
{
    struct blockdata_t* blockData = block->data.get();
    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
        blockData = block->data.get();
    }

    if (blockData->brackets.size()) {
        return;
    }

    if (lang->brackets) {
        std::vector<bracket_info_t> brackets;
        for (char* c = (char*)first; c < last;) {
            bool found = false;

            struct span_info_t span = spanAtBlock(blockData, c - first);
            if (span.length && (span.state == BLOCK_STATE_COMMENT || span.state == BLOCK_STATE_STRING)) {
                c++;
                continue;
            }

            // opening
            int i = 0;
            for (auto b : lang->bracketOpen) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block->lineNumber,
                        .position = l,
                        // .absolutePosition = block->position + l,
                        .bracket = i,
                        .open = true });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            // closing
            i = 0;
            for (auto b : lang->bracketClose) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block->lineNumber,
                        .position = l,
                        //.absolutePosition = block->position + l,
                        .bracket = i,
                        .open = false });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            c++;
        }

        blockData->brackets = brackets;

        // bracket pairing
        for (auto b : brackets) {
            if (!b.open && blockData->foldingBrackets.size()) {
                auto l = blockData->foldingBrackets.back();
                if (l.open && l.bracket == b.bracket) {
                    blockData->foldingBrackets.pop_back();
                } else {
                    // std::cout << "error brackets" << std::endl;
                }
                continue;
            }
            blockData->foldingBrackets.push_back(b);
        }

        // hack for if-else-
        if (blockData->foldingBrackets.size() == 2) {
            if (blockData->foldingBrackets[0].open != blockData->foldingBrackets[1].open && blockData->foldingBrackets[0].bracket == blockData->foldingBrackets[1].bracket) {
                blockData->foldingBrackets.clear();
                blockData->ifElseHack = true;
            }
        }

        if (blockData->foldingBrackets.size()) {
            auto l = blockData->foldingBrackets.back();
            blockData->foldable = l.open;
        }

        // log("brackets %d %d", blockData->brackets.size(), blockData->foldingBrackets.size());
    }
}

void* highlightThread(void* arg)
{
    highlighter_t* threadHl = (highlighter_t*)arg;
    editor_t* editor = threadHl->editor;

    editor_t tmp;
    tmp.document.open(editor->document.filePath, false);
    tmp.highlighter.lang = language_from_file(editor->document.filePath, app_t::instance()->extensions);
    // tmp.highlighter.lang = threadHl->lang;
    tmp.highlighter.theme = threadHl->theme;

    block_ptr b = tmp.document.firstBlock();
    while (b) {
        tmp.highlighter.highlightBlock(b);

        block_ptr sb = editor->snapshots[0].snapshot[b->lineNumber];
        sb->data = b->data;

        // log("%d", runLine);
        b = b->next();
        usleep(10);
    }

    usleep(1000);

    threadHl->threadId = 0;
    return NULL;
}

void highlighter_t::run(editor_t* editor)
{
    this->editor = editor;
    pthread_create(&threadId, NULL, &highlightThread, this);
}

void highlighter_t::cancel()
{
    if (threadId) {
        pthread_cancel(threadId);
        threadId = 0;
    }
}
