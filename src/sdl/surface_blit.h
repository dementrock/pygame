/*
  pygame - Python Game Library
  Copyright (C) 2009 Marcus von Appen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _PYGAME_SURFACEBLIT_H_
#define _PYGAME_SURFACEBLIT_H_

#include "surface.h"

/* The structure passed to the low level blit functions */
typedef struct
{
    Uint8          *s_pixels;
    int             s_width;
    int             s_height;
    int             s_skip;
    int             s_pitch;
    Uint8          *d_pixels;
    int             d_width;
    int             d_height;
    int             d_skip;
    int             d_pitch;
    void           *aux_data;
    SDL_PixelFormat *src;
    Uint8          *table;
    SDL_PixelFormat *dst;
} SDL_BlitInfo;

void blit_blend_rgb_add (SDL_BlitInfo* info);
void blit_blend_rgb_sub (SDL_BlitInfo* info);
void blit_blend_rgb_mul (SDL_BlitInfo* info);
void blit_blend_rgb_min (SDL_BlitInfo* info);
void blit_blend_rgb_max (SDL_BlitInfo* info);
void blit_blend_rgb_and (SDL_BlitInfo* info);
void blit_blend_rgb_or (SDL_BlitInfo* info);
void blit_blend_rgb_xor (SDL_BlitInfo* info);
void blit_blend_rgb_diff (SDL_BlitInfo* info);
void blit_blend_rgb_screen (SDL_BlitInfo* info);
void blit_blend_rgb_avg (SDL_BlitInfo* info);

void blit_blend_rgba_add (SDL_BlitInfo* info);
void blit_blend_rgba_sub (SDL_BlitInfo* info);
void blit_blend_rgba_mul (SDL_BlitInfo* info);
void blit_blend_rgba_min (SDL_BlitInfo* info);
void blit_blend_rgba_max (SDL_BlitInfo* info);
void blit_blend_rgba_and (SDL_BlitInfo* info);
void blit_blend_rgba_or (SDL_BlitInfo* info);
void blit_blend_rgba_xor (SDL_BlitInfo* info);
void blit_blend_rgba_diff (SDL_BlitInfo* info);
void blit_blend_rgba_screen (SDL_BlitInfo* info);
void blit_blend_rgba_avg (SDL_BlitInfo* info);

#endif /* _PYGAME_SURFACEBLIT_H_ */
