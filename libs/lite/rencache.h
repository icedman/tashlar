#ifndef RENCACHE_H
#define RENCACHE_H

#include "renderer.h"
#include <stdbool.h>

void rencache_show_debug(bool enable);
void rencache_free_font(RenFont* font);
void rencache_set_clip_rect(RenRect rect);
void rencache_draw_rect(RenRect rect, RenColor color);
int rencache_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold = false, bool italic = false);
void rencache_invalidate(void);
void rencache_begin_frame(void);
void rencache_end_frame(void);

#endif
