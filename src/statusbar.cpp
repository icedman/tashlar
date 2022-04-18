#include "statusbar.h"
#include "app.h"

static struct statusbar_t* statusbarInstance = 0;

statusbar_t::statusbar_t()
    : frames(0)
{
    statusbarInstance = this;
}

struct statusbar_t* statusbar_t::instance()
{
    return statusbarInstance;
}

void statusbar_t::setText(std::string s, int pos, int size)
{
    sizes[pos] = size;
    text[pos] = s;
}

void statusbar_t::setStatus(std::string s, int f)
{
    status = s;
    frames = (f + 500) * 1000;
}

void statusbar_t::update(int delta)
{
    if (frames < 0) {
        return;
    }

    frames = frames - delta;
    if (frames < 500) {
        status = "";
    }
}

