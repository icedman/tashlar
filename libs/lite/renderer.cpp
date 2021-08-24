#include "renderer.h"
#include "stb_truetype.h"
#include "stb_image.h"
#include "util.h"

#include <SDL2/SDL_ttf.h>

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_GLYPHSET 256

#define RENDER_MODE 2
// #define USE_SDL_TTF // better quality (doesn't support glyphsets)

void cache_glyphs(RenFont* font, int style);
TTF_Font* load_font(char* fname, int size);

static int sdlFontWidth = 1;
static int sdlFontHeight = 1;

struct RenImage {
    RenColor* pixels;
    int width, height;
};

typedef struct {
    RenImage* image;
#ifdef USE_SDL_TTF
    int minx,
        maxx,
        miny,
        maxy,
        advance;
#endif
    stbtt_bakedchar glyphs[256];
} GlyphSet;

struct RenFont {
    void* data;
    stbtt_fontinfo stbfont;

    GlyphSet* sets[MAX_GLYPHSET];
    GlyphSet* sets_bold[MAX_GLYPHSET];
    GlyphSet* sets_italic[MAX_GLYPHSET];

    float size;
    int height;

#ifdef USE_SDL_TTF
    TTF_Font* font;
#endif
};

static SDL_Window* window;
static struct {
    int left, top, right, bottom;
} clip;

static void* check_alloc(void* ptr)
{
    if (!ptr) {
        fprintf(stderr, "Fatal error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static const char* utf8_to_codepoint(const char* p, unsigned* dst)
{
    unsigned res, n;
    switch (*p & 0xf0) {
    case 0xf0:
        res = *p & 0x07;
        n = 3;
        break;
    case 0xe0:
        res = *p & 0x0f;
        n = 2;
        break;
    case 0xd0:
    case 0xc0:
        res = *p & 0x1f;
        n = 1;
        break;
    default:
        res = *p;
        n = 0;
        break;
    }
    while (n--) {
        res = (res << 6) | (*(++p) & 0x3f);
    }
    *dst = res;
    return p + 1;
}

void ren_init(SDL_Window* win)
{
    assert(win);
    window = win;
    SDL_Surface* surf = SDL_GetWindowSurface(window);
    ren_set_clip_rect((RenRect){ 0, 0, surf->w, surf->h });
}

void ren_update_rects(RenRect* rects, int count)
{
    SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*)rects, count);
    static bool initial_frame = true;
    if (initial_frame) {
        SDL_ShowWindow(window);
        initial_frame = false;
    }
}

void ren_set_clip_rect(RenRect rect)
{
    clip.left = rect.x;
    clip.top = rect.y;
    clip.right = rect.x + rect.width;
    clip.bottom = rect.y + rect.height;
}

void ren_get_size(int* x, int* y)
{
    SDL_Surface* surf = SDL_GetWindowSurface(window);
    *x = surf->w;
    *y = surf->h;
}

RenImage* ren_new_image(int width, int height)
{
    assert(width > 0 && height > 0);
    RenImage* image = (RenImage*)malloc(sizeof(RenImage) + width * height * sizeof(RenColor));
    check_alloc(image);
    image->pixels = (RenColor*)((void*)(image + 1));
    image->width = width;
    image->height = height;
    return image;
}

void ren_free_image(RenImage* image)
{
    free(image);
}

static GlyphSet* load_glyphset(RenFont* font, int idx)
{
    GlyphSet* set = (GlyphSet*)check_alloc(calloc(1, sizeof(GlyphSet)));

    /* init image */
    int width = 128;
    int height = 128;
retry:
    set->image = ren_new_image(width, height);

    /* load glyphs */
    float s = stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) / stbtt_ScaleForPixelHeight(&font->stbfont, 1);
    int res = stbtt_BakeFontBitmap(
        (const unsigned char*)font->data, 0, font->size * s, (unsigned char*)((void*)set->image->pixels),
        width, height, idx * 256, 256, set->glyphs);

    /* retry with a larger image buffer if the buffer wasn't large enough */
    if (res < 0) {
        width *= 2;
        height *= 2;
        ren_free_image(set->image);
        goto retry;
    }

    /* adjust glyph yoffsets and xadvance */
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
    float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
    int scaled_ascent = ascent * scale + 0.5;
    for (int i = 0; i < 256; i++) {
        set->glyphs[i].yoff += scaled_ascent;
        set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
    }

    /* convert 8bit data to 32bit */
    for (int i = width * height - 1; i >= 0; i--) {
        uint8_t n = *((uint8_t*)set->image->pixels + i);
        set->image->pixels[i] = (RenColor){ .b = 255, .g = 255, .r = 255, .a = n };
    }

    return set;
}

static GlyphSet* get_glyphset(RenFont* font, int codepoint)
{
    int idx = (codepoint >> 8) % MAX_GLYPHSET;
    if (!font->sets[idx]) {
        font->sets[idx] = load_glyphset(font, idx);
    }
    return font->sets[idx];
}

RenFont* ren_load_font(const char* filename, float size)
{
    RenFont* font = NULL;

#ifdef USE_SDL_TTF
    /* cache new glyphs */
    font = (RenFont*)check_alloc(calloc(1, sizeof(RenFont)));
    font->size = size;

    font->font = load_font((char*)filename, size);

    if (!font->font) {
        free(font);
        return NULL;
    }

    cache_glyphs(font, TTF_STYLE_NORMAL);
    cache_glyphs(font, TTF_STYLE_BOLD);
    cache_glyphs(font, TTF_STYLE_ITALIC);

    return font;
#else
    FILE* fp = NULL;

    /* init font */
    font = (RenFont*)check_alloc(calloc(1, sizeof(RenFont)));
    font->size = size;

    /* load font into buffer */
    fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }
    /* get size */
    fseek(fp, 0, SEEK_END);
    int buf_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    /* load */
    font->data = check_alloc(malloc(buf_size));
    int _ = fread(font->data, 1, buf_size, fp);
    (void)_;
    fclose(fp);
    fp = NULL;

    /* init stbfont */
    int ok = stbtt_InitFont(&font->stbfont, (const unsigned char*)font->data, 0);
    if (ok) {

        /* get height and scale */
        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
        float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, size);
        font->height = (ascent - descent + linegap) * scale + 0.5;

        /* make tab and newline glyphs invisible */
        stbtt_bakedchar* g = get_glyphset(font, '\n')->glyphs;
        g['\t'].x1 = g['\t'].x0;
        g['\n'].x1 = g['\n'].x0;

        return font;
    }

    if (fp) {
        fclose(fp);
    }
    if (font) {
        free(font->data);
    }
    free(font);
#endif
    return NULL;
}

void ren_free_font(RenFont* font)
{
#ifdef USE_SDL_TTF
    TTF_CloseFont(font->font);
#endif

    for (int i = 0; i < MAX_GLYPHSET; i++) {
        GlyphSet* set = font->sets[i];
        if (set) {
            ren_free_image(set->image);
            free(set);
        }
    }
    free(font->data);
    free(font);
}

void ren_set_font_tab_width(RenFont* font, int n)
{
#ifdef USE_SDL_TTF
    //
#else
    GlyphSet* set = get_glyphset(font, '\t');
    set->glyphs['\t'].xadvance = n;
#endif
}

int ren_get_font_width(RenFont* font, const char* text)
{
#ifdef USE_SDL_TTF
    int x = 0;
    const char* p = text;
    unsigned codepoint;
    while (*p) {
        p = utf8_to_codepoint(p, &codepoint);
        if (codepoint > 128) return 0;
        x += font->sets[codepoint]->advance;
    }

    return x;
#else
    int x = 0;
    const char* p = text;
    unsigned codepoint;
    while (*p) {
        p = utf8_to_codepoint(p, &codepoint);
        GlyphSet* set = get_glyphset(font, codepoint);
        stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];
        x += g->xadvance;
    }
    return x;
#endif
}

int ren_get_font_height(RenFont* font)
{
#ifdef USE_SDL_TTF
    return sdlFontHeight;
#else
    return font->height;
#endif
}

static inline RenColor blend_pixel(RenColor dst, RenColor src)
{
    int ia = 0xff - src.a;
    dst.r = ((src.r * src.a) + (dst.r * ia)) >> 8;
    dst.g = ((src.g * src.a) + (dst.g * ia)) >> 8;
    dst.b = ((src.b * src.a) + (dst.b * ia)) >> 8;
    return dst;
}

static inline RenColor blend_pixel2(RenColor dst, RenColor src, RenColor color)
{
    src.a = (src.a * color.a) >> 8;
    int ia = 0xff - src.a;
    dst.r = ((src.r * color.r * src.a) >> 16) + ((dst.r * ia) >> 8);
    dst.g = ((src.g * color.g * src.a) >> 16) + ((dst.g * ia) >> 8);
    dst.b = ((src.b * color.b * src.a) >> 16) + ((dst.b * ia) >> 8);
    return dst;
}

#define rect_draw_loop(expr)            \
    for (int j = y1; j < y2; j++) {     \
        for (int i = x1; i < x2; i++) { \
            *d = expr;                  \
            d++;                        \
        }                               \
        d += dr;                        \
    }

void ren_draw_rect(RenRect rect, RenColor color)
{
    if (color.a == 0) {
        return;
    }

    int x1 = rect.x < clip.left ? clip.left : rect.x;
    int y1 = rect.y < clip.top ? clip.top : rect.y;
    int x2 = rect.x + rect.width;
    int y2 = rect.y + rect.height;
    x2 = x2 > clip.right ? clip.right : x2;
    y2 = y2 > clip.bottom ? clip.bottom : y2;

    SDL_Surface* surf = SDL_GetWindowSurface(window);
    RenColor* d = (RenColor*)surf->pixels;
    d += x1 + y1 * surf->w;
    int dr = surf->w - (x2 - x1);

    if (color.a == 0xff) {
        rect_draw_loop(color);
    } else {
        rect_draw_loop(blend_pixel(*d, color));
    }
}

void ren_draw_image(RenImage* image, RenRect* sub, int x, int y, RenColor color)
{
    if (color.a == 0) {
        return;
    }

    /* clip */
    int n;
    if ((n = clip.left - x) > 0) {
        sub->width -= n;
        sub->x += n;
        x += n;
    }
    if ((n = clip.top - y) > 0) {
        sub->height -= n;
        sub->y += n;
        y += n;
    }
    if ((n = x + sub->width - clip.right) > 0) {
        sub->width -= n;
    }
    if ((n = y + sub->height - clip.bottom) > 0) {
        sub->height -= n;
    }

    if (sub->width <= 0 || sub->height <= 0) {
        return;
    }

    /* draw */
    SDL_Surface* surf = SDL_GetWindowSurface(window);
    RenColor* s = image->pixels;
    RenColor* d = (RenColor*)surf->pixels;
    s += sub->x + sub->y * image->width;
    d += x + y * surf->w;
    int sr = image->width - sub->width;
    int dr = surf->w - sub->width;

    for (int j = 0; j < sub->height; j++) {
        for (int i = 0; i < sub->width; i++) {
            *d = blend_pixel2(*d, *s, color);
            d++;
            s++;
        }
        d += dr;
        s += sr;
    }
}

int ren_draw_wtext(RenFont* font, const wchar_t* text, int x, int y, RenColor color, bool bold, bool italic)
{
    SDL_Color fg = { 255, 255, 255, 255 };
    #ifdef USE_SDL_TTF
        SDL_Surface *surf = TTF_RenderUTF8_Blended(font->font, (const char*)text, fg);   
        SDL_Surface* wsurf = SDL_GetWindowSurface(window);
        if(surf) {
            SDL_Rect rect;
            rect.x = x;
            rect.y = y;
            rect.w = surf->w;
            rect.h = surf->h;
            SDL_Rect srect;
            rect.x = 0;
            rect.y = 0;
            rect.w = surf->w;
            rect.h = surf->h;
            SDL_BlitSurface(surf, &srect, wsurf, &rect); 
            SDL_FreeSurface(surf);
        }
    #endif
    return 1;
}

int ren_draw_text(RenFont* font, const char* text, int x, int y, RenColor color, bool bold, bool italic)
{
    RenRect rect;
    const char* p = text;
    unsigned codepoint;
    while (*p) {
        unsigned cp = *p;
        p = utf8_to_codepoint(p, &codepoint);

        if (codepoint >= 128 || !isprint(codepoint)) {
            codepoint = '?';
        }

#ifdef USE_SDL_TTF
        GlyphSet** sets = font->sets;
        if (bold) {
            sets = font->sets_bold;
        }
        if (italic) {
            sets = font->sets_italic;
        }
        if (sets[codepoint]) {
            RenImage* image = sets[codepoint]->image;
            rect.x = 0;
            rect.y = 0;
            rect.width = image->width;
            rect.height = image->height;
            ren_draw_image(image, &rect, x, y, color);
            x += sets[codepoint]->advance;
        }
#else
        GlyphSet* set = get_glyphset(font, codepoint);
        stbtt_bakedchar* g = &set->glyphs[codepoint & 0xff];
        rect.x = g->x0;
        rect.y = g->y0;
        rect.width = g->x1 - g->x0;
        rect.height = g->y1 - g->y0;
        ren_draw_image(set->image, &rect, x + g->xoff, y + g->yoff, color);
        x += sdlFontWidth; // g->xadvance;
#endif
    }
    return x;
}

#ifdef USE_SDL_TTF
TTF_Font* load_font(char* fname, int size)
{
    TTF_Init();

    char* p;

    TTF_Font* font = TTF_OpenFont(fname, size);
    if (!font) {
        log("TTF_OpenFont: %s", TTF_GetError());
        return NULL;
    }

    /* print some metrics and attributes */
    log("size                    : %d\n", size);
    log("TTF_FontHeight          : %d\n", TTF_FontHeight(font));
    log("TTF_FontAscent          : %d\n", TTF_FontAscent(font));
    log("TTF_FontDescent         : %d\n", TTF_FontDescent(font));
    log("TTF_FontLineSkip        : %d\n", TTF_FontLineSkip(font));
    log("TTF_FontFaceIsFixedWidth: %d\n", TTF_FontFaceIsFixedWidth(font));
    {
        char* str = TTF_FontFaceFamilyName(font);
        if (!str)
            str = "(null)";
        log("TTF_FontFaceFamilyName  : \"%s\"\n", str);
    }
    {
        char* str = TTF_FontFaceStyleName(font);
        if (!str)
            str = "(null)";
        log("TTF_FontFaceStyleName   : \"%s\"\n", str);
    }
    if (TTF_GlyphIsProvided(font, 'g')) {
        int minx, maxx, miny, maxy, advance;
        TTF_GlyphMetrics(font, 'g', &minx, &maxx, &miny, &maxy, &advance);
        log("TTF_GlyphMetrics('g'):\n\tminx=%d\n\tmaxx=%d\n\tminy=%d\n\tmaxy=%d\n\tadvance=%d\n",
            minx, maxx, miny, maxy, advance);
    } else
        log("TTF_GlyphMetrics('g'): unavailable in font!\n");

    /* set window title and icon name, using filename and stuff */
    p = strrchr(fname, '/');
    if (!p)
        p = strrchr(fname, '\\');
    if (!p)
        p = strrchr(fname, ':');
    if (!p)
        p = fname;
    else
        p++;

    return font;
}

static Uint32 get_pixel(SDL_Surface* surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16*)p;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32*)p;
        break;

    default:
        return 0; /* shouldn't happen, but avoids warnings */
    }
}

void cache_glyphs(RenFont* renFont, int style)
{
    int start_glyph = 0,
        kerning = 1,
        hinting = TTF_HINTING_NORMAL,
        outline = 0,
        font_size = 32;

    TTF_Font* font = renFont->font;

    log("cache_glyphs");

    int i;

    SDL_Color fg = { 255, 255, 255, 255 };
#if RENDER_MODE == 1
    SDL_Color bg = { 255, 255, 255, 255 };
#endif

    if (!font)
        return;

    if (style != TTF_GetFontStyle(font))
        TTF_SetFontStyle(font, style);
    if (kerning != !!TTF_GetFontKerning(font))
        TTF_SetFontKerning(font, kerning);
    if (hinting != TTF_GetFontHinting(font))
        TTF_SetFontHinting(font, hinting);
    if (outline != TTF_GetFontOutline(font))
        TTF_SetFontOutline(font, outline);

    int ww = 0;
    int hh = 0;
    int ii = 0;

    for (i = 0; i < 128; i++) {
        char _p[] = { (char)i, 0 };
        unsigned codepoint = 0;
        utf8_to_codepoint(_p, &codepoint);
        SDL_Surface* surf = NULL;

        GlyphSet* set = (GlyphSet*)check_alloc(calloc(1, sizeof(GlyphSet)));
        switch (style) {
        case TTF_STYLE_NORMAL:
            renFont->sets[codepoint] = set;
            break;
        case TTF_STYLE_BOLD:
            renFont->sets_bold[codepoint] = set;
            break;
        case TTF_STYLE_ITALIC:
            renFont->sets_italic[codepoint] = set;
            break;
        }

        /* cache rendered surface */
#if RENDER_MODE == 0
        surf = TTF_RenderGlyph_Solid(font, i + start_glyph, fg);
#elif RENDER_MODE == 1
        surf = TTF_RenderGlyph_Shaded(font, i + start_glyph, fg, bg);
#elif RENDER_MODE == 2
        surf = TTF_RenderGlyph_Blended(font, i + start_glyph, fg);
#endif

        if (!surf) {
            log("TTF_RenderGlyph_Shaded: %d %s\n", i, TTF_GetError());
            continue;
        }

        /* cache metrics */
        TTF_GlyphMetrics(font, i + start_glyph,
            &set->minx, &set->maxx,
            &set->miny, &set->maxy,
            &set->advance);

        int _w = surf->w;
        int _h = surf->h;
        int bpp = surf->format->BytesPerPixel;

        ww += set->advance;
        hh += _h;
        ii++;

        set->image = ren_new_image(_w, _h);

        for (int y = 0; y < _h; y++) {
            for (int x = 0; x < _w; x++) {
                SDL_Color rgb;
                Uint32 pixel = get_pixel(surf, x, y);
                SDL_GetRGBA(pixel, surf->format, &rgb.r, &rgb.g, &rgb.b, &rgb.a);
                RenColor clr = {
                    .b = rgb.b,
                    .g = rgb.g,
                    .r = rgb.r,
                    .a = rgb.a
                };
                set->image->pixels[y * _w + x] = clr;
            }
        }

        SDL_FreeSurface(surf);
    }

    sdlFontWidth = ww / ii;
    sdlFontHeight = hh / ii;
}
#endif
