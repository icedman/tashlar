#include "window.h"

static int windowId = 1000;

window_t::window_t(bool f)
    : viewX(0)
    , viewY(0)
    , scrollX(0)
    , scrollY(0)
    , win(0)
{
    id = windowId++;
    focusable = f;
}

void window_t::layout(int w, int h)
{
}

void window_t::render()
{
}

void window_t::renderCursor()
{}

bool window_t::processCommand(command_e cmd, char ch)
{
    return false;
}