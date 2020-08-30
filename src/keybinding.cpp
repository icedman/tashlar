#include "keybinding.h"
#include "extension.h"
#include "util.h"

typedef struct {
    const char* name;
    operation_e op;
    const char* keys;
} operation_name;

static const operation_name operation_names[] = {
    { "UNKNOWN", UNKNOWN, "" },
    { "CANCEL", CANCEL, "escape" },

    { "NEW_TAB ", NEW_TAB, "alt+t" },
    { "TAB_1", TAB_1, "alt+1" },
    { "TAB_2", TAB_2, "alt+2" },
    { "TAB_3", TAB_3, "alt+3" },
    { "TAB_4", TAB_4, "alt+4" },
    { "TAB_5", TAB_5, "alt+5" },
    { "TAB_6", TAB_6, "alt+6" },
    { "TAB_7", TAB_7, "alt+7" },
    { "TAB_8", TAB_8, "alt+8" },
    { "TAB_9", TAB_9, "alt+9" },

    { "TAB", TAB, "tab" },
    { "ENTER", ENTER, "enter" },
    { "DELETE", DELETE, "delete" },
    { "BACKSPACE", BACKSPACE, "backspace" },
    { "INSERT", INSERT, "" },

    { "CUT", CUT, "ctrl+x" },
    { "COPY", COPY, "ctrl+c" },
    { "PASTE", PASTE, "ctrl+v" },
    { "SELECT_WORD", SELECT_WORD, "" },

    { "INDENT", INDENT, "ctrl+l+ctrl+k" },
    { "UNINDENT", UNINDENT, "ctrl+l+ctrl+j" },
    { "TOGGLE_COMMENT", TOGGLE_COMMENT, "ctrl+k+ctrl+k" },

    { "SELECT_ALL", SELECT_ALL, "ctrl+a" },
    { "SELECT_LINE", SELECT_LINE, "ctrl+l+ctrl+l" },
    { "DUPLICATE_LINE", DUPLICATE_LINE, "ctrl+l+ctrl+v" },
    { "DELETE_LINE", DELETE_LINE, "ctrl+l+ctrl+x" },
    { "MOVE_LINE_UP", MOVE_LINE_UP, "ctrl+l+ctrl+up" },
    { "MOVE_LINE_DOWN", MOVE_LINE_DOWN, "ctrl+l+ctrl+down" },

    { "CLEAR_SELECTION", CLEAR_SELECTION, "" },
    { "DUPLICATE_SELECTION", DUPLICATE_SELECTION, "" },
    { "DELETE_SELECTION", DELETE_SELECTION, "" },
    { "ADD_CURSOR_AND_MOVE_UP", ADD_CURSOR_AND_MOVE_UP, "ctrl+up" },
    { "ADD_CURSOR_AND_MOVE_DOWN", ADD_CURSOR_AND_MOVE_DOWN, "ctrl+down" },
    { "ADD_CURSOR_FOR_SELECTED_WORD", ADD_CURSOR_FOR_SELECTED_WORD, "ctrl+d" },
    { "CLEAR_CURSORS", CLEAR_CURSORS, "" },

    { "MOVE_CURSOR", MOVE_CURSOR, "" },
    { "MOVE_CURSOR_LEFT", MOVE_CURSOR_LEFT, "left" },
    { "MOVE_CURSOR_RIGHT", MOVE_CURSOR_RIGHT, "right" },
    { "MOVE_CURSOR_UP", MOVE_CURSOR_UP, "up" },
    { "MOVE_CURSOR_DOWN", MOVE_CURSOR_DOWN, "down" },
    { "MOVE_CURSOR_NEXT_WORD", MOVE_CURSOR_NEXT_WORD, "ctrl+right" },
    { "MOVE_CURSOR_PREVIOUS_WORD", MOVE_CURSOR_PREVIOUS_WORD, "ctrl+left" },
    { "MOVE_CURSOR_START_OF_LINE", MOVE_CURSOR_START_OF_LINE, "home" },
    { "MOVE_CURSOR_END_OF_LINE", MOVE_CURSOR_END_OF_LINE, "end" },
    { "MOVE_CURSOR_START_OF_DOCUMENT", MOVE_CURSOR_START_OF_DOCUMENT, "ctrl+home" },
    { "MOVE_CURSOR_END_OF_DOCUMENT", MOVE_CURSOR_END_OF_DOCUMENT, "ctrl+end" },
    { "MOVE_CURSOR_NEXT_PAGE", MOVE_CURSOR_NEXT_PAGE, "pagedown" },
    { "MOVE_CURSOR_PREVIOUS_PAGE", MOVE_CURSOR_PREVIOUS_PAGE, "pageup" },

    { "MOVE_CURSOR_ANCHORED", MOVE_CURSOR_ANCHORED, "" },
    { "MOVE_CURSOR_LEFT_ANCHORED", MOVE_CURSOR_LEFT_ANCHORED, "shift+left" },
    { "MOVE_CURSOR_RIGHT_ANCHORED", MOVE_CURSOR_RIGHT_ANCHORED, "shift+right" },
    { "MOVE_CURSOR_UP_ANCHORED", MOVE_CURSOR_UP_ANCHORED, "shift+up" },
    { "MOVE_CURSOR_DOWN_ANCHORED", MOVE_CURSOR_DOWN_ANCHORED, "shift+down" },
    { "MOVE_CURSOR_NEXT_WORD_ANCHORED", MOVE_CURSOR_NEXT_WORD_ANCHORED, "ctrl+shift+right" },
    { "MOVE_CURSOR_PREVIOUS_WORD_ANCHORED", MOVE_CURSOR_PREVIOUS_WORD_ANCHORED, "ctrl+shift+left" },
    { "MOVE_CURSOR_START_OF_LINE_ANCHORED", MOVE_CURSOR_START_OF_LINE_ANCHORED, "shift+home" },
    { "MOVE_CURSOR_END_OF_LINE_ANCHORED", MOVE_CURSOR_END_OF_LINE_ANCHORED, "shift+end" },
    { "MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED", MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED, "ctrl+shift+home" },
    { "MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED", MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED, "ctrl+shift+end" },
    { "MOVE_CURSOR_NEXT_PAGE_ANCHORED", MOVE_CURSOR_NEXT_PAGE_ANCHORED, "shift+pagedown" },
    { "MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED", MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED, "shift+pageup" },

    { "MOVE_FOCUS_LEFT", MOVE_FOCUS_LEFT, "ctrl+alt+left" },
    { "MOVE_FOCUS_RIGHT", MOVE_FOCUS_RIGHT, "ctrl+alt+right" },
    { "MOVE_FOCUS_UP", MOVE_FOCUS_UP, "ctrl+alt+up" },
    { "MOVE_FOCUS_DOWN", MOVE_FOCUS_DOWN, "ctrl+alt+down" },

    { "SPLIT_VIEW", SPLIT_VIEW, "ctrl+k+ctrl+t" },
    { "TOGGLE_FOLD", TOGGLE_FOLD, "ctrl+l+ctrl+f" },

    { "OPEN", OPEN, "" },
    { "SAVE", SAVE, "ctrl+s" },
    { "SAVE_AS", SAVE_AS, "" },
    { "SAVE_COPY", SAVE_COPY, "" },
    { "UNDO", UNDO, "ctrl+z" },
    { "REDO", REDO, "" },
    { "CLOSE", CLOSE, "alt+w" },
    { "QUIT", QUIT, "ctrl+q" },

    { "POPUP_SEARCH", POPUP_SEARCH, "ctrl+f" },
    { "POPUP_SEARCH_LINE", POPUP_SEARCH_LINE, "ctrl+g" },
    { "POPUP_COMMANDS", POPUP_COMMANDS, "ctrl+o" },
    { "POPUP_FILES", POPUP_FILES, "ctrl+p" },
    { "POPUP_COMPLETION", POPUP_COMPLETION, "" },

    { 0 }
};

operation_e operationFromName(std::string name)
{
    for (auto op : keybinding_t::instance()->binding) {
        if (op.name == name) {
            return op.op;
        }
    }
    return UNKNOWN;
}

operation_e operationFromKeys(std::string keys)
{
    for (auto op : keybinding_t::instance()->binding) {
        if (op.keys == keys) {
            return op.op;
        }
    }
    return UNKNOWN;
}

std::string nameFromOperation(operation_e _op)
{
    for (auto op : keybinding_t::instance()->binding) {
        if (op.op == _op) {
            return op.name;
        }
    }
    return "unknown";
}

static struct keybinding_t* keybindingInstance = 0;

struct keybinding_t* keybinding_t::instance()
{
    return keybindingInstance;
}

keybinding_t::keybinding_t()
{
    keybindingInstance = this;
}

keybinding_t::~keybinding_t()
{
}

void keybinding_t::initialize()
{
    for (int i = 0; operation_names[i].name; i++) {
        operation_t op;
        op.name = operation_names[i].name;
        op.keys = operation_names[i].keys;
        op.op = operation_names[i].op;
        binding.push_back(op);
    }

    // override
    std::string _path = "~/.ashlar/keybinding.json";

    char* cpath = (char*)malloc(_path.length() + 1 * sizeof(char));
    strcpy(cpath, _path.c_str());
    expand_path((char**)(&cpath));
    const std::string path(cpath);
    free(cpath);

    Json::Value settings = parse::loadJson(path);
    if (settings.isArray()) {
        for (auto cmd : settings) {
            if (!cmd.isObject())
                continue;
            std::string keys = cmd["keys"].asString();
            std::string command = cmd["command"].asString();
            for (auto& bind : binding) {
                if (bind.name == command) {
                    bind.keys = keys;
                    break;
                }
            }
        }
    }
}
