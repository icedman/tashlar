#include "highlighter.h"
#include "app.h"
#include "document.h"
#include "parse.h"

#include <cstring>

highlighter_t::highlighter_t()
{
}

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos)
{
    span_info_t res;
    res.length = 0;
    for (auto span : blockData->spans) {
        if (span.length == 0) {
            continue;
        }
        if (pos >= span.start && pos < span.start + span.length) {
            res = span;
            if (res.state != BLOCK_STATE_UNKNOWN) {
                return res;
            }
        }
    }

    return res;
}

static void setFormatFromStyle(size_t start, size_t length, style_t& style, const char* line, struct blockdata_t* blockData, std::string scope)
{
    int s = -1;

    block_state_e state;
    if (scope.find("comment") != std::string::npos) {
        state = BLOCK_STATE_COMMENT;
    } else if (scope.find("string") != std::string::npos) {
        state = BLOCK_STATE_STRING;
    } else {
        state = BLOCK_STATE_UNKNOWN;
    }

    for (int i = start; i < start + length; i++) {
        if (s == -1) {
            if (line[i] != ' ' && line[i] != '\t') {
                s = i;
            }
            continue;
        }
        if (line[i] == ' ' || i + 1 == start + length) {
            if (s != -1) {
                span_info_t span = {
                    .start = s,
                    .length = i - s + 1,
                    .colorIndex = style.foreground.index,
                    .state = state
                };
                blockData->spans.insert(blockData->spans.begin(), 1, span);
            }
            s = -1;
        }
    }
}

void highlighter_t::highlightBlock(block_ptr block)
{
    if (!lang) {
        return;
    }

    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    if (!block->data->dirty) {
        return;
    }

    // app_t::instance()->log("highlight %d", block.uid);

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
    blockData->scopes.clear();
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

    if (text.length() > 500) {
        // too long to parse
    } else {
        parser_state = parse::parse(first, last, parser_state, scopes, firstLine);
    }

    std::string prevScopeName;
    size_t si = 0;
    size_t n = 0;
    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    while (it != scopes.end()) {
        n = it->first;
        scope::scope_t scope = it->second;

        // if (settings->debug_scopes) {
        blockData->scopes.emplace(n, scope);
        // }

        std::string scopeName = to_s(scope);
        it++;

        if (n > si) {
            style_t s = theme->styles_for_scope(prevScopeName);
            setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
        }

        prevScopeName = scopeName;
        // if (prevScopeName.find("variable") != std::string::npos) {
        //    prevScopeName = "parameter";
        // }
        si = n;
    }

    n = last - first;
    if (n > si) {
        style_t s = theme->styles_for_scope(prevScopeName);
        setFormatFromStyle(si, n - si, s, first, blockData, prevScopeName);
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
            setFormatFromStyle(b, e - b, s, first, blockData, "comment");
        } else {
            blockData->state = BLOCK_STATE_UNKNOWN;
            if (endComment != std::string::npos && previousBlockState == BLOCK_STATE_COMMENT) {
                setFormatFromStyle(0, endComment + lang->blockCommentEnd.length(), s, first, blockData, "comment");
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
            //if (nextBlockData && blockData->lastRule != nextBlockData->lastPrevBlockRule) {
            //    nextBlockData->dirty = true;
            //}

            if (nextBlockData) {
                nextBlockData->dirty = true;
            }
        }
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

        // format brackets with scope
        // style_t s = theme->styles_for_scope("bracket");
        // for (auto b : blockData->brackets) {
        // setFormatFromStyle(b.position, 1, s, first, blockData, "bracket");
        // }

        if (blockData->foldingBrackets.size()) {
            auto l = blockData->foldingBrackets.back();
            blockData->foldable = l.open;
        }

        // app_t::instance()->log("brackets %d %d", blockData->brackets.size(), blockData->foldingBrackets.size());
    }
}

void* hl(void* arg)
{
    editor_t* editor = (editor_t*)arg;
    document_t* doc = &editor->document;

    block_ptr blk = editor->hlTarget;
    if (!blk)
        return NULL;

    int idx = 0;
    while (blk) {
        app_t::log("hl:%d", blk->lineNumber);
        editor->highlighter.highlightBlock(blk);
        blk = blk->next();
        if (idx++ > 200)
            break;
    }

    editor->hlTarget = nullptr;
    return NULL;
}

void highlighter_t::run(editor_t* editor)
{
    pthread_create(&threadId, NULL, &hl, editor);
}
