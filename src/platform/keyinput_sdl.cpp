#include "keyinput.h"

static bool lastConsumed = false;
static char lastKey = ' ';
static std::string lastKeySequence = "resize";

void pushKey(char c, std::string keySequence)
{
    lastConsumed = false;
    lastKey = c;
    lastKeySequence = keySequence;
}

int readKey(std::string& keySequence)
{
    if (lastConsumed) {
        keySequence = "";
        return -1;
    }

    keySequence = lastKeySequence;
    lastConsumed = true;
    return lastKey;
}