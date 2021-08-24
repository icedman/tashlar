#include "sdl_stbimage.h"

// #ifdef SDL_STBIMAGE_IMPLEMENTATION

// make stb_image use SDL_malloc etc, so SDL_FreeSurface() can SDL_free()
// the data allocated by stb_image
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free
#define STB_IMAGE_IMPLEMENTATION
#ifndef SDL_STBIMG_ALLOW_STDIO
  #define STBI_NO_STDIO // don't need STDIO, will use SDL_RWops to open files
#endif
#include "stb_image.h"

typedef struct {
   unsigned char* data;
   int w;
   int h;
   int format; // 3: RGB, 4: RGBA
} STBIMG__image;

static SDL_Surface* STBIMG__CreateSurfaceImpl(STBIMG__image img, int freeWithSurface)
{
   SDL_Surface* surf = NULL;
   Uint32 rmask, gmask, bmask, amask;
   // ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use
   // a void* for the data, but it's really treated as endian-specific Uint32*
   // and there isn't even an SDL_PIXELFORMAT_* for 32bit byte-wise RGBA
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   int shift = (img.format == STBI_rgb) ? 8 : 0;
   rmask = 0xff000000 >> shift;
   gmask = 0x00ff0000 >> shift;
   bmask = 0x0000ff00 >> shift;
   amask = 0x000000ff >> shift;
#else // little endian, like x86
   rmask = 0x000000ff;
   gmask = 0x0000ff00;
   bmask = 0x00ff0000;
   amask = (img.format == STBI_rgb) ? 0 : 0xff000000;
#endif

   surf = SDL_CreateRGBSurfaceFrom((void*)img.data, img.w, img.h,
                                   img.format*8, img.format*img.w,
                                   rmask, gmask, bmask, amask);

   if(surf == NULL)
   {
      // hopefully SDL_CreateRGBSurfaceFrom() has set an sdl error
      return NULL;
   }

   if(freeWithSurface)
   {
      // SDL_Surface::flags is documented to be read-only.. but if the pixeldata
      // has been allocated with SDL_malloc()/SDL_calloc()/SDL_realloc() this
      // should work (and it currently does) + @icculus said it's reasonably safe:
      //  https://twitter.com/icculus/status/667036586610139137 :-)
      // clear the SDL_PREALLOC flag, so SDL_FreeSurface() free()s the data passed from img.data
      surf->flags &= ~SDL_PREALLOC;
   }

   return surf;
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_LoadFromMemory(const unsigned char* buffer, int length)
{
   STBIMG__image img = {0};
   int bppToUse = 0;
   int inforet = 0;
   SDL_Surface* ret = NULL;

   if(buffer == NULL)
   {
      SDL_SetError("STBIMG_LoadFromMemory(): passed buffer was NULL!");
      return NULL;
   }
   if(length <= 0)
   {
      SDL_SetError("STBIMG_LoadFromMemory(): passed invalid length: %d!", length);
      return NULL;
   }

   inforet = stbi_info_from_memory(buffer, length, &img.w, &img.h, &img.format);
   if(!inforet)
   {
      SDL_SetError("STBIMG_LoadFromMemory(): Couldn't get image info: %s!\n", stbi_failure_reason());
      return NULL;
   }

   // no alpha => use RGB, else use RGBA
   bppToUse = (img.format == STBI_grey || img.format == STBI_rgb) ? STBI_rgb : STBI_rgb_alpha;

   img.data = stbi_load_from_memory(buffer, length, &img.w, &img.h, &img.format, bppToUse);
   if(img.data == NULL)
   {
      SDL_SetError("STBIMG_LoadFromMemory(): Couldn't load image: %s!\n", stbi_failure_reason());
      return NULL;
   }
   img.format = bppToUse;

   ret = STBIMG__CreateSurfaceImpl(img, 1);

   if(ret == NULL)
   {
      // no need to log an error here, it was an SDL error which should still be available through SDL_GetError()
      SDL_free(img.data);
      return NULL;
   }

   return ret;
}


// fill 'data' with 'size' bytes.  return number of bytes actually read
static int STBIMG__io_read(void* user, char* data, int size)
{
   STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;

   int ret = SDL_RWread(io->src, data, sizeof(char), size);
   if(ret == 0)
   {
      // we're at EOF or some error happend
      io->atEOF = 1;
   }
   return (int)ret*sizeof(char);
}

// skip the next 'n' bytes, or 'unget' the last -n bytes if negative
static void STBIMG__io_skip(void* user, int n)
{
   STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;

   if(SDL_RWseek(io->src, n, RW_SEEK_CUR) == -1)
   {
      // an error happened during seeking, hopefully setting EOF will make stb_image abort
      io->atEOF = 2; // set this to 2 for "aborting because seeking failed" (stb_image only cares about != 0)
   }
}

// returns nonzero if we are at end of file/data
static int STBIMG__io_eof(void* user)
{
   STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;
   return io->atEOF;
}


SDL_STBIMG_DEF SDL_bool STBIMG_stbi_callback_from_RW(SDL_RWops* src, STBIMG_stbio_RWops* out)
{
   if(out == NULL)
   {
      SDL_SetError("STBIMG_stbi_callback_from_RW(): out must not be NULL!");
      return SDL_FALSE;
   }

   // make sure out is at least initialized to something deterministic
   memset(out, 0, sizeof(*out));

   if(src == NULL)
   {
      SDL_SetError("STBIMG_stbi_callback_from_RW(): src must not be NULL!");
      return SDL_FALSE;
   }

   out->src = src;
   out->atEOF = 0;
   out->stb_cbs.read = STBIMG__io_read;
   out->stb_cbs.skip = STBIMG__io_skip;
   out->stb_cbs.eof  = STBIMG__io_eof;

   return SDL_TRUE;
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW(SDL_RWops* src, int freesrc)
{
   STBIMG__image img = {0};
   int bppToUse = 0;
   int inforet = 0;
   SDL_Surface* ret = NULL;
   Sint64 srcOffset = 0;

   STBIMG_stbio_RWops cbData;

   if(src == NULL)
   {
      SDL_SetError("STBIMG_Load_RW(): src was NULL!");
      return NULL;
   }

   srcOffset = SDL_RWtell(src);
   if(srcOffset < 0)
   {
      SDL_SetError("STBIMG_Load_RW(): src must be seekable, maybe use STBIMG_Load_RW_noSeek() instead!");
      // TODO: or do that automatically? but I think the user should be aware of what they're doing
      goto end;
   }

   if(!STBIMG_stbi_callback_from_RW(src, &cbData))
   {
      goto end;
   }

   inforet = stbi_info_from_callbacks(&cbData.stb_cbs, &cbData, &img.w, &img.h, &img.format);
   if(!inforet)
   {
      if(cbData.atEOF == 2) SDL_SetError("STBIMG_Load_RW(): src must be seekable!");
      else SDL_SetError("STBIMG_Load_RW(): Couldn't get image info: %s!\n", stbi_failure_reason());
      goto end;
   }

   // rewind src so stbi_load_from_callbacks() will start reading from the beginning again
   if(SDL_RWseek(src, srcOffset, RW_SEEK_SET) < 0)
   {
      SDL_SetError("STBIMG_Load_RW(): src must be seekable!");
      goto end;
   }

   cbData.atEOF = 0; // we've rewinded (rewound?)

   // no alpha => use RGB, else use RGBA
   bppToUse = (img.format == STBI_grey || img.format == STBI_rgb) ? STBI_rgb : STBI_rgb_alpha;

   img.data = stbi_load_from_callbacks(&cbData.stb_cbs, &cbData, &img.w, &img.h, &img.format, bppToUse);
   if(img.data == NULL)
   {
      SDL_SetError("STBIMG_Load_RW(): Couldn't load image: %s!\n", stbi_failure_reason());
      goto end;
   }
   img.format = bppToUse;

   ret = STBIMG__CreateSurfaceImpl(img, 1);

   if(ret == NULL)
   {
      // no need to log an error here, it was an SDL error which should still be available through SDL_GetError()
      SDL_free(img.data);
      img.data = NULL;
      goto  end;
   }

end:
   if(freesrc)
   {
      SDL_RWclose(src);
   }
   else if(img.data == NULL)
   {
      // if data is still NULL, there was an error and we should probably
      // seek src back to where it was when this function was called
      SDL_RWseek(src, srcOffset, RW_SEEK_SET);
   }

   return ret;
}

#if SDL_MAJOR_VERSION > 1
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW_noSeek(SDL_RWops* src, int freesrc)
{
   unsigned char* buf = NULL;
   Sint64 fileSize = 0;
   SDL_Surface* ret = NULL;

   if(src == NULL)
   {
      SDL_SetError("STBIMG_Load_RW_noSeek(): src was NULL!");
      return NULL;
   }

   fileSize = SDL_RWsize(src);
   if(fileSize < 0)
   {
      goto end; // SDL should have set an error already
   }
   else if(fileSize == 0)
   {
      SDL_SetError("STBIMG_Load_RW_noSeek(): SDL_RWsize(src) returned 0 => empty file/stream?!");
      goto end;
   }
   else if(fileSize > 0x7FFFFFFF)
   {
      // stb_image.h uses ints for all sizes, so we can't support more
      // (but >2GB images are insane anyway)
      SDL_SetError("STBIMG_Load_RW_noSeek(): SDL_RWsize(src) too big (> 2GB)!");
      goto end;
   }

   buf = (unsigned char*)SDL_malloc(fileSize);
   if(buf == NULL)
   {
      SDL_SetError("STBIMG_Load_RW_noSeek(): Couldn't allocate buffer to read src into!");
      goto end;
   }

   if(SDL_RWread(src, buf, fileSize, 1) > 0)
   {
      // if that fails, STBIMG_LoadFromMemory() has set an SDL error
      // and ret is NULL, so nothing special to do for us
      ret = STBIMG_LoadFromMemory(buf, fileSize);
   }

end:
   if(freesrc)
   {
      SDL_RWclose(src);
   }

   SDL_free(buf);
   return ret;
}
#endif // SDL_MAJOR_VERSION > 1


SDL_STBIMG_DEF SDL_Surface* STBIMG_Load(const char* file)
{
   SDL_RWops* src = SDL_RWFromFile(file, "rb");
   if(src == NULL) return NULL;
   return STBIMG_Load_RW(src, 1);
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_CreateSurface(unsigned char* pixelData, int width, int height, int bytesPerPixel, SDL_bool freeWithSurface)
{
   STBIMG__image img;

   if(pixelData == NULL)
   {
      SDL_SetError("STBIMG_CreateSurface(): passed pixelData was NULL!");
      return NULL;
   }
   if(bytesPerPixel != 3 && bytesPerPixel != 4)
   {
      SDL_SetError("STBIMG_CreateSurface(): passed bytesPerPixel = %d, only 3 (24bit RGB) and 4 (32bit RGBA) are allowed!", bytesPerPixel);
      return NULL;
   }
   if(width <= 0 || height <= 0)
   {
      SDL_SetError("STBIMG_CreateSurface(): width and height must be > 0!");
      return NULL;
   }

   img.data = pixelData;
   img.w = width;
   img.h = height;
   img.format = bytesPerPixel;

   return STBIMG__CreateSurfaceImpl(img, freeWithSurface);
}

#if SDL_MAJOR_VERSION > 1
static SDL_Texture* STBIMG__SurfToTex(SDL_Renderer* renderer, SDL_Surface* surf)
{
   SDL_Texture* ret = NULL;
   if(surf != NULL)
   {
      ret = SDL_CreateTextureFromSurface(renderer, surf);
      SDL_FreeSurface(surf); // not needed anymore, it's copied into tex
   }
   // if surf is NULL, whatever tried to create it should have called SDL_SetError(),
   // if SDL_CreateTextureFromSurface() returned NULL it should have set an error
   // so whenever this returns NULL, the user should be able to get a useful
   // error-message with SDL_GetError().
   return ret;
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture(SDL_Renderer* renderer, const char* file)
{
   return STBIMG__SurfToTex(renderer, STBIMG_Load(file));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTextureFromMemory(SDL_Renderer *renderer, const unsigned char* buffer, int length)
{
   return STBIMG__SurfToTex(renderer, STBIMG_LoadFromMemory(buffer, length));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW(SDL_Renderer* renderer, SDL_RWops* src, int freesrc)
{
   return STBIMG__SurfToTex(renderer, STBIMG_Load_RW(src, freesrc));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_CreateTexture(SDL_Renderer* renderer, const unsigned char* pixelData,
                     int width, int height, int bytesPerPixel)
{
   SDL_Surface* surf = STBIMG_CreateSurface((unsigned char*)pixelData, width, height, bytesPerPixel, SDL_FALSE);
   return STBIMG__SurfToTex(renderer, surf);
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW_noSeek(SDL_Renderer* renderer, SDL_RWops* src, int freesrc)
{
   return STBIMG__SurfToTex(renderer, STBIMG_Load_RW_noSeek(src, freesrc));
}
#endif // SDL_MAJOR_VERSION > 1

// #endif // SDL_STBIMAGE_IMPLEMENTATION
