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
#ifndef _PYGAME_FREETYPE_H_
#define _PYGAME_FREETYPE_H_

#include "pgbase.h"

#include <ft2build.h>  
#include FT_FREETYPE_H 
#include FT_CACHE_H
#include FT_XFREE86_H

#ifdef __cplusplus
extern "C" {
#endif

#define PYGAME_FREETYPE_FIRSTSLOT 0
#define PYGAME_FREETYPE_NUMSLOTS 0

#ifndef PYGAME_FREETYPE_INTERNAL
#endif /* PYGAME_FREETYPE_INTERNAL */

typedef struct
{
    int face_index;
    FT_Open_Args open_args;
} FontId;

typedef struct
{
    PyFont pyfont;
    FontId id;
    int default_ptsize;
} PyFreeTypeFont;

#define PyFreeTypeFont_AsFont(x) (((PyFreeTypeFont *)x)->font)

#define PYGAME_FREETYPE_FONT_FIRSTSLOT \
    (PYGAME_FREETYPE_FIRSTSLOT + PYGAME_FREETYPE_NUMSLOTS)

#define PYGAME_FREETYPE_FONT_NUMSLOTS 2

#ifndef PYGAME_FREETYPE_FONT_INTERNAL
#   define PyFreeTypeFont_Type \
    (*(PyTypeObject*)PyGameFreeType_C_API[PYGAME_FREETYPE_FONT_FIRSTSLOT + 0])

#   define PyFreeTypeFont_Check(x) \
    (PyObject_TypeCheck(x, \
        (PyTypeObject*)PyGameFreeType_C_API[PYGAME_FREETYPE_FONT_FIRSTSLOT + 0]))

#   define PyFreeTypeFont_New \
    (*(PyObject*(*)(const char*,int))PyGameFreeType_C_API[PYGAME_FREETYPE_FONT_FIRSTSLOT + 1])
#endif /* PYGAME_FREETYPE_FONT_INTERNAL */

/*
 * C API export.
 */
#ifdef PYGAME_INTERNAL
    void **PyGameFreeType_C_API;
#else
    static void **PyGameFreeType_C_API;
#endif

#define PYGAME_FREETYPE_SLOTS \
    (PYGAME_FREETYPE_FONT_FIRSTSLOT + PYGAME_FREETYPE_FONT_NUMSLOTS)

#define PYGAME_FREETYPE_ENTRY "_PYGAME_FREETYPE_CAPI"

static int
import_pygame2_freetype_base(void)
{
    PyObject *_module = PyImport_ImportModule("pygame2.freetype.base");
    if (_module != NULL)
    {
        PyObject *_capi = PyObject_GetAttrString(_module, PYGAME_FREETYPE_ENTRY);
        if (!PyCObject_Check (_capi))
        {
            Py_DECREF (_module);
            return -1;
        }

        PyGameFreeType_C_API = (void **)PyCObject_AsVoidPtr(_capi);
        Py_DECREF(_capi);
        return 0;
    }

    return -1;
}

#ifdef __cplusplus
}
#endif

#endif /* _PYGAME_FONTS_H_ */
