#include <stdio.h>
#include "app.h"
#include "search.h"
#include "explorer.h"
#include "editor.h"
#include "operation.h"

#include "render.h"

void render(editor_ptr editor)
{
	int scrollX = 0;
	int scrollY = 0;
	int rows = 32;
	int cols = 1000;

	editor->highlight(scrollY, rows);
	document_t *doc = &editor->document;

	cursor_list cursors = doc->cursors;
   	cursor_t mainCursor = doc->cursor();

	block_list::iterator it= doc->blocks.begin();
    if (scrollY > 0) {
        it += scrollY;
    }

    int _foldedLines = 0;
    int l = 0;

    bool hlMainCursor = cursors.size() == 1 && !mainCursor.hasSelection();
    bool firstLine = true;
    while (it != doc->blocks.end()) {
        auto& b = *it++;

        if (l >= rows + 1)
            break;

        int colorPair = color_pair_e::NORMAL;
        int colorPairSelected = color_pair_e::SELECTED;

        size_t lineNo = b->lineNumber;
        struct blockdata_t* blockData = b->data.get();

        std::string text = b->text() + " ";

        char* line = (char*)text.c_str();
        for (int sl = 0; sl < b->lineCount; sl++) {
        	printf("\n");
            // _move(l, 0);
            // _clrtoeol(cols);
            // _move(l++, 0);
            l++;

            if (text.length() < scrollX) {
                continue;
            }

            if (blockData && blockData->folded && !blockData->foldable) {
                l--;
                _foldedLines += b->lineCount;
                continue;
            }

            if (l >= rows + 1)
                break;

            // log("%s", line);
            int col = 0;
            for (int i = scrollX;; i++) {
                size_t pos = i + (sl * cols);
                if (col++ >= cols) {
                    break;
                }

                char ch = line[pos];
                if (ch == 0)
                    break;

                bool hl = false;
                bool ul = false;
                bool bl = false; // bold
                bool il = false; // italic

                colorPair = color_pair_e::NORMAL;
                colorPairSelected = color_pair_e::SELECTED;

                // syntax here
                if (blockData) {
                    struct span_info_t span = spanAtBlock(blockData, pos);
                    bl = span.bold;
                    il = span.italic;
                    if (span.length) {
                        colorPair = pairForColor(span.colorIndex, false);
                        colorPairSelected = pairForColor(span.colorIndex, true);
                    }
                }

                // bracket
                if (editor->cursorBracket1.bracket != -1 && editor->cursorBracket2.bracket != -1) {
                    if ((pos == editor->cursorBracket1.position && lineNo == editor->cursorBracket1.line) ||
                    	(pos == editor->cursorBracket2.position && lineNo == editor->cursorBracket2.line)) {
                        // _underline(true);
                    }
                }

                // cursors
                for (auto& c : cursors) {
                    if (pos == c.position() && b == c.block()) {
                        hl = true;
                        ul = c.hasSelection();
                        bl = c.hasSelection();
                    }
                    if (!c.hasSelection())
                        continue;

                    cursor_position_t start = c.selectionStart();
                    cursor_position_t end = c.selectionEnd();
                    if (b->lineNumber < start.block->lineNumber || b->lineNumber > end.block->lineNumber)
                        continue;
                    if (b == start.block && pos < start.position)
                        continue;
                    if (b == end.block && pos > end.position)
                        continue;
                    hl = true;
                    break;
                }

                if (hl) {
                    colorPair = colorPairSelected;
                }
                if (ul) {
                    // _underline(true);
                }
                if (bl) {
                    // _bold(true);
                }
                if (il) {
                    // _italic(true);
                }

                // _attron(_color_pair(colorPair));
                // _addch(ch);
                printf("%d%c", colorPair,ch);
                // _attroff(_color_pair(colorPair));
                // _bold(false);
                // _italic(false);
                // _underline(false);
            }
        }
    }

    while (l < rows) {
    	printf("\n~");
        // _move(l, 0);
        // _clrtoeol(cols);
        // _move(l++, 0);
        // addch('~');
        l++;
    }

    printf("\n");
}

int main(int argc, char **argv)
{
	app_t app;
    keybinding_t keybinding;
    explorer_t explorer;
    search_t search;

	app.configure(argc, argv);
    app.setupColors();
    
	std::string file = "./src/main.cpp";
    if (argc > 1) {
        file = argv[argc - 1];
    }

    app.openEditor(file);    	
	// app.currentEditor->document.print();

	explorer_t::instance()->setRootFromFile(file);

	editor_ptr editor = app.currentEditor; // std::make_shared<editor_t>();
	document_t *doc = &editor->document;

	editor->pushOp({
		operation_e::MOVE_CURSOR, "", "", ""
	});
	editor->pushOp({
		operation_e::MOVE_CURSOR_RIGHT, "1", "", ""
	});
	// editor->pushOp({
	// 	operation_e::SELECT_WORD, "1", "", ""
	// });
	// editor->pushOp({
	// 	operation_e::DELETE, "1", "", ""
	// });
	editor->pushOp({
		operation_e::INSERT,
		"HELLO WoRLD",
		"",""
	});
	editor->runAllOps();

	// doc->print();

	render(editor);

	// explorer.print();
	return 0;
}