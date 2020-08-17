#include "keyinput.h"


static bool lastConsumed = true;
static char lastKey = '?';
static std::string lastKeySequence;

void pushKey(char c, std::string keySequence)
{
    lastConsumed = false;
    lastKey = c;
    lastKeySequence = keySequence;
}

int kbhit(int timeout)
{
    return 0;
}

int readKey(std::string& keySequence)
{
    if (lastConsumed) {
        keySequence = "";
        return -2;
    }

    keySequence = lastKeySequence;
    lastConsumed = true;
    return lastKey;
}