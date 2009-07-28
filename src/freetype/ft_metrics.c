/*
  pygame - Python Game Library
  Copyright (C) 2009 Vicent Marti

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

#define PYGAME_FREETYPE_INTERNAL

#include "ft_mod.h"
#include "ft_wrap.h"
#include "pgfreetype.h"
#include "pgtypes.h"
#include "freetypebase_doc.h"

#include FT_MODULE_H

void _PGFT_GetMetrics_INTERNAL(FT_Glyph glyph, FT_UInt bbmode,
    int *minx, int *maxx, int *miny, int *maxy, int *advance)
{
    FT_BBox box;
    FT_Glyph_Get_CBox(glyph, bbmode, &box);

    *minx = box.xMin;
    *maxx = box.xMax;
    *miny = box.yMin;
    *maxy = box.yMax;
    *advance = glyph->advance.x;

    if (bbmode == FT_GLYPH_BBOX_TRUNCATE ||
        bbmode == FT_GLYPH_BBOX_PIXELS)
        *advance >>= 16;
}


int PGFT_GetMetrics(FreeTypeInstance *ft, PyFreeTypeFont *font,
    int character, const FontRenderMode *render, int bbmode,
    void *minx, void *maxx, void *miny, void *maxy, void *advance)
{
    FT_Error        error;
    FTC_ScalerRec   scale;
    FontGlyph *     glyph = NULL;


    glyph = PGFT_Cache_FindGlyph(ft, &PGFT_INTERNALS(font)->cache, 
            (FT_UInt)character, render);

    if (!glyph)
        return -1;

    _PGFT_GetMetrics_INTERNAL(glyph->image, (FT_UInt)bbmode, 
            minx, maxx, miny, maxy, advance);

    if (bbmode == FT_BBOX_EXACT || bbmode == FT_BBOX_EXACT_GRIDFIT)
    {
        *(float *)minx =    (FP_266_FLOAT(*(int *)minx));
        *(float *)miny =    (FP_266_FLOAT(*(int *)miny));
        *(float *)maxx =    (FP_266_FLOAT(*(int *)maxx));
        *(float *)maxy =    (FP_266_FLOAT(*(int *)maxy));
        *(float *)advance = (FP_1616_FLOAT(*(int *)advance));
    }

    return 0;
}


int
_PGFT_GetTextSize_INTERNAL(FreeTypeInstance *ft, PyFreeTypeFont *font, 
    const FontRenderMode *render, FontText *text)
{
    FT_Vector   extent, advances[MAX_GLYPHS];
    FT_Error    error;
    FT_Vector   size;

    error = PGFT_GetTextAdvances(ft, font, render, text, advances);

    if (error)
        return error;

    extent = advances[text->length - 1];

    if (render->render_flags & FT_RFLAG_VERTICAL)
    {
        size.x = text->glyph_size.x;
        size.y = ABS(extent.y);
    }
    else
    {
        size.x = extent.x;
        size.y = text->glyph_size.y;
    }

    if (render->rotation_angle)
    {   
        size.x = MAX(size.x, size.y);
        size.y = MAX(size.x, size.y);
    }

    text->text_size.x = ABS(size.x);
    text->text_size.y = ABS(size.y);

    return 0;
}

int
PGFT_GetTextSize(FreeTypeInstance *ft, PyFreeTypeFont *font,
    const FontRenderMode *render, PyObject *text, int *w, int *h)
{
    FontText *font_text;

    font_text = PGFT_LoadFontText(ft, font, render, text);

    if (!font_text)
        return -1;

    if (_PGFT_GetTextSize_INTERNAL(ft, font, render, font_text) != 0)
        return -1;

    *w = PGFT_TRUNC(PGFT_ROUND(font_text->text_size.x));
    *h = PGFT_TRUNC(PGFT_ROUND(font_text->text_size.y));
    return 0;
}
