/*
 * A small header-only library to load an image into a RGB(A) SDL_Surface*,
 * like a stripped down version of SDL_Image, but using stb_image.h to decode
 * images and thus without any further external dependencies.
 * Supports all filetypes supported by stb_image (JPEG, PNG, TGA, BMP, PSD, ...
 * See stb_image.h for details).
 *
 * (C) 2015 Daniel Gibson
 *
 * Homepage: https://github.com/DanielGibson/Snippets/
 *
 * Dependencies:
 *     libSDL2      http://www.libsdl.org
 *     stb_image.h  https://github.com/nothings/stb
 *
 * Usage:
 *   Put this file and stb_image.h somewhere in your project.
 *   In *one* of your .c/.cpp files, do
 *     #define SDL_STBIMAGE_IMPLEMENTATION
 *     #include "SDL_stbimage.h"
 *   to create the implementation of this library in that file.
 *   You can just #include "SDL_stbimage.h" (without the #define) in other source
 *   files to use it there. (See also below this comment for an usage example)
 *   This header implicitly #includes <SDL.h> and "stb_image.h".
 *
 *   You can #define SDL_STBIMG_DEF before including this header if you want to
 *   prepend anything to the function signatures (like "static", "inline",
 *   "__declspec(dllexport)", ...)
 *     Example: #define SDL_STBIMG_DEF static inline
 *
 *   By default, this deactivates stb_image's load from file functions via
 *   #define STBI_NO_STDIO, as they use stdio.h  and that adds a dependency to the
 *   CRT on windows and with SDL you're better off using SDL_RWops, incl. SDL_RWFromFile()
 *   If you wanna use stbi_load(), stbi_info(), stbi_load_from_file() etc anyway, do
 *     #define SDL_STBIMG_ALLOW_STDIO
 *   before including this header.
 *   (Note that all the STBIMG_* functions of this lib will work without it)
 *
 *   stb_image.h uses assert.h by default. You can #define STBI_ASSERT(x)
 *   before the implementation-#include of SDL_stbimage.h to avoid that.
 *   By default stb_image supports HDR images, for that it needs pow() from libm.
 *   If you don't need HDR (it can't be loaded into a SDL_Surface anyway),
 *   #define STBI_NO_LINEAR and #define STBI_NO_HDR before including this header.
 *
 * License:
 *   This software is dual-licensed to the public domain and under the following
 *   license: you are granted a perpetual, irrevocable license to copy, modify,
 *   publish, and distribute this file as you see fit.
 *   No warranty implied; use at your own risk.
 *
 * So you can do whatever you want with this code, including copying it
 * (or parts of it) into your own source.
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */

#if 0 // Usage Example:
  #define SDL_STBIMAGE_IMPLEMENTATION
  #include "SDL_stbimage.h"

  void yourFunction(const char* imageFilePath)
  {
    SDL_Surface* surf = STBIMG_Load(imageFilePath);
    if(surf == NULL) {
      printf("ERROR: Couldn't load %s, reason: %s\n", imageFilePath, SDL_GetError());
      exit(1);
    }

    // ... do something with surf ...

    SDL_FreeSurface(surf);
  }
#endif // 0 (usage example)


#ifndef SDL__STBIMAGE_H
#define SDL__STBIMAGE_H

// if you really think you need <SDL2/SDL.h> here instead.. feel free to change it,
// but the cool kids have path/to/include/SDL2/ in their compilers include path.
#include <SDL.h>

#ifndef SDL_STBIMG_ALLOW_STDIO
  #define STBI_NO_STDIO // don't need STDIO, will use SDL_RWops to open files
#endif
#include "stb_image.h"

// this allows you to prepend stuff to function signatures, e.g. "static"
#ifndef SDL_STBIMG_DEF
  // by default it's empty
  #define SDL_STBIMG_DEF
#endif // DG_MISC_DEF


#ifdef __cplusplus
extern "C" {
#endif

// loads the image file at the given path into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load(const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_LoadFromMemory(const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Surface from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW(SDL_RWops* src, int freesrc);


// Creates an SDL_Surface* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// If freeWithSurface is SDL_TRUE, SDL_FreeSurface() will free the pixelData
//  you passed with SDL_free() - NOTE that you should only do that if pixelData
//  was allocated with SDL_malloc(), SDL_calloc() or SDL_realloc()!
// Returns NULL on error (in that case pixelData won't be freed!),
//  use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_CreateSurface(unsigned char* pixelData, int width, int height,
                                                 int bytesPerPixel, SDL_bool freeWithSurface);


#if SDL_MAJOR_VERSION > 1
// loads the image file at the given path into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture(SDL_Renderer* renderer, const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTextureFromMemory(SDL_Renderer* renderer, const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Texture from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW(SDL_Renderer* renderer, SDL_RWops* src, int freesrc);

// Creates an SDL_Texture* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_CreateTexture(SDL_Renderer* renderer, const unsigned char* pixelData,
                     int width, int height, int bytesPerPixel);
#endif // SDL_MAJOR_VERSION > 1


typedef struct {
   SDL_RWops* src;
   stbi_io_callbacks stb_cbs;
   int atEOF; // defaults to 0; 1: reached EOF or error on read, 2: error on seek
} STBIMG_stbio_RWops;

// creates stbi_io_callbacks and userdata to use stbi_*_from_callbacks() directly,
//  especially useful to use SDL_RWops with stb_image, without using SDL_Surface
// src must be readable and seekable!
// Returns SDL_FALSE on error (SDL_GetError() will give you info), else SDL_TRUE
// NOTE: If you want to use src twice (e.g. for info and load), remember to rewind
//       it by seeking back to its initial position and resetting out->atEOF to 0
//       inbetween the uses!
SDL_STBIMG_DEF SDL_bool STBIMG_stbi_callback_from_RW(SDL_RWops* src, STBIMG_stbio_RWops* out);

#if 0 //  Use STBIMG_stbi_callback_from_RW() like this:
  SDL_RWops* src = ...; // wherever it's from
  STBIMG_stbio_RWops io;
  if(!STBIMG_stbi_callback_from_RW(src, &io)) {
    printf("ERROR creating stbio callbacks: %s\n", SDL_GetError());
    exit(1);
  }
  Sint64 origSrcPosition = SDL_RWtell(src);
  int w, h, fmt;
  if(!stbi_info_from_callbacks(&io.stb_cbs, &io, &w, &h, &fmt)) {
     printf("stbi_info_from_callbacks() failed, reason: %s\n", stbi_failure_reason());
     exit(1);
  }
  printf("image is %d x %d pixels with %d bytes per pixel\n", w, h, fmt);

  // rewind src before using it again in stbi_load_from_callbacks()
  if(SDL_RWseek(src, origSrcPosition, RW_SEEK_SET) < 0)
  {
    printf("ERROR: src not be seekable!\n");
    exit(1);
  }
  io.atEOF = 0; // remember to reset atEOF, too!

  unsigned char* data;
  data = stbi_load_from_callbacks(&io.stb_cbs, &io, &w, &h, &fmt, 0);
  if(data == NULL) {
    printf("stbi_load_from_callbacks() failed, reason: %s\n", stbi_failure_reason());
    exit(1);
  }
  // ... do something with data ...
  stbi_image_free(data);
#endif // 0 (STBIMG_stbi_callback_from_RW() example)


#if SDL_MAJOR_VERSION > 1
// loads an image file into a RGB(A) SDL_Surface from a SDL_RWops (src)
// - without using SDL_RWseek(), for streams that don't support or are slow
//   at seeking. It reads everything into a buffer and calls STBIMG_LoadFromMemory()
// You should probably only use this if you *really* have performance problems
//  because of seeking or your src doesn't support  SDL_RWseek(), but SDL_RWsize()
// src must at least support SDL_RWread() and SDL_RWsize()
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW_noSeek(SDL_RWops* src, int freesrc);

// the same for textures (you should probably not use this one, either..)
SDL_STBIMG_DEF SDL_Texture* STBIMG_LoadTexture_RW_noSeek(SDL_Renderer* renderer, SDL_RWops* src, int freesrc);
#endif // SDL_MAJOR_VERSION > 1

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDL__STBIMAGE_H


// ############# Below: Implementation ###############

