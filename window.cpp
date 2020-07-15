#include "window.h"
#include "app.h"

static int windowId = 1000;

window_t::window_t(bool f)
    : viewX(0)
    , viewY(0)
    , scrollX(0)
    , scrollY(0)
    , win(0)
    , shown(false)
{
    id = windowId++;
    focusable = f;
}

void window_t::renderCursor()
{
}

bool window_t::processCommand(command_e cmd, char ch)
{
    return false;
}

bool window_t::isFocused()
{
    return app_t::instance()->focused->id == id;
}

bool window_t::isShown()
{
    return isFocused() || shown;
}

void window_t::update(int frames)
{
}