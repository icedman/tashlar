#include "render.h"

render_t* render_t::instance()
{
    return NULL;
}

bool render_t::isTerminal()
{
    return false;
}

std::string render_t::getClipboardText()
{
    return "";
}

void render_t::setClipboardText(std::string text)
{}

int render_t::pairForColor(int colorIdx, bool selected)
{
    return colorIdx;
}