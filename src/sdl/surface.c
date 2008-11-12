/*
  pygame - Python Game Library
  Copyright (C) 2000-2001 Pete Shinners, 2007-2008 Marcus von Appen

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
#define PYGAME_SDLSURFACE_INTERNAL

#include "videomod.h"
#include "surface.h"
#include "pgsdl.h"
#include "tga.h"

#ifdef HAVE_PNG
#include "png.h"
#endif

static PyObject* _surface_new (PyTypeObject *type, PyObject *args,
    PyObject *kwds);
static int _surface_init (PyObject *surface, PyObject *args, PyObject *kwds);
static void _surface_dealloc (PySurface *self);

static PyObject* _surface_getdict (PyObject *self, void *closure);
static PyObject* _surface_getcliprect (PyObject *self, void *closure);
static int _surface_setcliprect (PyObject *self, PyObject *value,
    void *closure);
static PyObject* _surface_getsize (PyObject *self, void *closure);
static PyObject* _surface_getflags (PyObject *self, void *closure);
static PyObject* _surface_getformat (PyObject *self, void *closure);
static PyObject* _surface_getpitch (PyObject *self, void *closure);
static PyObject* _surface_getpixels (PyObject *self, void *closure);
static PyObject* _surface_getwidth (PyObject *self, void *closure);
static PyObject* _surface_getheight (PyObject *self, void *closure);
static PyObject* _surface_getlocked (PyObject *self, void *closure);

static PyObject* _surface_update (PyObject *self, PyObject *args);
static PyObject* _surface_flip (PyObject *self);
static PyObject* _surface_setcolors (PyObject *self, PyObject *args);
static PyObject* _surface_getpalette (PyObject *self);
static PyObject* _surface_setpalette (PyObject *self, PyObject *args);
static PyObject* _surface_lock (PyObject *self);
static PyObject* _surface_unlock (PyObject *self);
static PyObject* _surface_getcolorkey (PyObject *self);
static PyObject* _surface_setcolorkey (PyObject *self, PyObject *args);
static PyObject* _surface_getalpha (PyObject *self);
static PyObject* _surface_setalpha (PyObject *self, PyObject *args);
static PyObject* _surface_convert (PyObject *self, PyObject *args,
    PyObject *kwds);
static PyObject* _surface_clone (PyObject *self);
static PyObject* _surface_blit (PyObject *self, PyObject *args, PyObject *kwds);
static PyObject* _surface_fill (PyObject *self, PyObject *args, PyObject *kwds);
static PyObject* _surface_save (PyObject *self, PyObject *args);
static PyObject* _surface_getat (PyObject *self, PyObject *args);
static PyObject* _surface_setat (PyObject *self, PyObject *args);

static void _release_c_lock (void *ptr);

/**
 */
static PyMethodDef _surface_methods[] = {
    { "update", _surface_update, METH_VARARGS, "" },
    { "flip", (PyCFunction)_surface_flip, METH_NOARGS, "" },
    { "set_colors", _surface_setcolors, METH_VARARGS, "" },
    { "get_palette", (PyCFunction) _surface_getpalette, METH_NOARGS, "" },
    { "set_palette", _surface_setpalette, METH_VARARGS, "" },
    { "lock", (PyCFunction)_surface_lock, METH_NOARGS, "" },
    { "unlock", (PyCFunction)_surface_unlock, METH_NOARGS, "" },
    { "get_colorkey", (PyCFunction) _surface_getcolorkey, METH_NOARGS, "" },
    { "set_colorkey", _surface_setcolorkey, METH_VARARGS, "" },
    { "get_alpha", (PyCFunction) _surface_getalpha, METH_NOARGS, "" },
    { "set_alpha", _surface_setalpha, METH_VARARGS, "" },
    { "get_at", _surface_getat, METH_VARARGS, "" },
    { "set_at", _surface_setat, METH_VARARGS, "" },
    { "convert", (PyCFunction) _surface_convert, METH_VARARGS | METH_KEYWORDS,
        "" },
    { "clone", (PyCFunction)_surface_clone, METH_NOARGS, "" },
    { "blit", (PyCFunction)_surface_blit, METH_VARARGS | METH_KEYWORDS, "" },
    { "fill", (PyCFunction)_surface_fill, METH_VARARGS | METH_KEYWORDS, "" },
    { "save", _surface_save, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

/**
 */
static PyGetSetDef _surface_getsets[] = {
    { "__dict__", _surface_getdict, NULL, "", NULL },
    { "clip_rect", _surface_getcliprect, _surface_setcliprect, "", NULL },
    { "w", _surface_getwidth, NULL, "", NULL },
    { "width", _surface_getwidth, NULL, "", NULL },
    { "h", _surface_getheight, NULL, "", NULL },
    { "height", _surface_getheight, NULL, "", NULL },
    { "size", _surface_getsize, NULL, "", NULL },
    { "flags", _surface_getflags, NULL, "", NULL },
    { "format", _surface_getformat, NULL, "", NULL },
    { "pitch", _surface_getpitch, NULL, "", NULL },
    { "pixels", _surface_getpixels, NULL, "", NULL },
    { "locked", _surface_getlocked, NULL, "", NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

/**
 */
PyTypeObject PySurface_Type =
{
    TYPE_HEAD(NULL, 0)
    "video.Surface",              /* tp_name */
    sizeof (PySurface),   /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor) _surface_dealloc, /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS,
    "",
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof (PySurface, weakrefs), /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    _surface_methods,           /* tp_methods */
    0,                          /* tp_members */
    _surface_getsets,           /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof (PySurface, dict), /* tp_dictoffset */
    (initproc) _surface_init,   /* tp_init */
    0,                          /* tp_alloc */
    _surface_new,               /* tp_new */
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
#if PY_VERSION_HEX >= 0x02060000
    0                           /* tp_version_tag */
#endif
};

static void
_surface_dealloc (PySurface *self)
{
    Py_XDECREF (self->dict);
    Py_XDECREF (self->locklist);
    if (self->weakrefs)
        PyObject_ClearWeakRefs ((PyObject *) self);

    if (self->surface)
        SDL_FreeSurface (self->surface);

    ((PyObject*)self)->ob_type->tp_free ((PyObject *) self);
}

static PyObject*
_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySurface *surface = (PySurface *)type->tp_alloc (type, 0);
    if (!surface)
        return NULL;

    surface->locklist = NULL;
    surface->dict = NULL;
    surface->weakrefs = NULL;
    surface->intlocks = 0;
    return (PyObject*) surface;
}

static int
_surface_init (PyObject *self, PyObject *args, PyObject *kwds)
{
    Uint32 flags = 0;
    int width, height, depth = 0;
    Uint32 rmask, gmask, bmask, amask;
    SDL_Surface *surface;
    PyObject *masks = NULL;

    ASSERT_VIDEO_INIT (-1);

    static char *keys[] = { "width", "height", "depth", "flags", "masks",
                            NULL };
    if (!PyArg_ParseTupleAndKeywords (args, kwds, "ii|ilO", keys, &width,
            &height, &depth, &flags, &masks))
        return -1;

    if (width < 0 || height < 0)
    {
        PyErr_SetString (PyExc_ValueError,
            "width and height must not be negative");
        return -1;
    }

    if (!depth && !masks && !flags)
    {
        /* Use the optimal video depth or set default depth. */
        const SDL_VideoInfo *info = SDL_GetVideoInfo ();
        depth = info->vfmt->BitsPerPixel;
    }
    
    if (depth && !masks)
    {
        /* Using depth we let the screen resolution decide about the
         * rgba mask order */
        rmask = gmask = bmask = amask = 0;

        if (flags & SDL_SRCALPHA)
        {
            /* The user wants an alpha component. In that case we will
             * set a default RGBA mask. */
            switch (depth)
            {
            case 16:
                amask = 0xf000;
                rmask = 0x0f00;
                gmask = 0x00f0;
                bmask = 0x000f;
                break;
            case 32:
                amask = 0xff000000;
                rmask = 0x00ff0000;
                gmask = 0x0000ff00;
                bmask = 0x000000ff;
                break;
            default:
                PyErr_SetString (PyExc_PyGameError,
                    "Per-pixel alpha not supported for that depth");
                return -1;
            }
        }
    }
    else if (masks)
    {
        if (!PySequence_Check (masks) || PySequence_Size (masks) != 4)
        {
            PyErr_SetString (PyExc_ValueError,
                "masks must be a 4-value sequence");
            return -1;
        }
        if (!Uint32FromSeqIndex (masks, 0, &rmask) ||
            !Uint32FromSeqIndex (masks, 1, &gmask) ||
            !Uint32FromSeqIndex (masks, 2, &bmask) ||
            !Uint32FromSeqIndex (masks, 3, &amask))
        {
            PyErr_SetString (PyExc_ValueError,
                "invalid mask values in masks sequence");
            return -1;
        }
    }
    
    surface = SDL_CreateRGBSurface (flags, width, height, depth, rmask, gmask,
        bmask, amask);
    if (!surface)
        return -1;

    ((PySurface*)self)->surface = surface;
    return 0;
}

/* Surface getters/setters */
static PyObject*
_surface_getdict (PyObject *self, void *closure)
{
    PySurface *surface = (PySurface*) self;
    if (!surface->dict)
    {
        surface->dict = PyDict_New ();
        if (!surface->dict)
            return NULL;
    }
    Py_INCREF (surface->dict);
    return surface->dict;
}

static PyObject*
_surface_getcliprect (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    SDL_Rect sdlrect;

    SDL_GetClipRect (surface, &sdlrect);
    return PyRect_New (sdlrect.x, sdlrect.y, sdlrect.w, sdlrect.h);
}

static int
_surface_setcliprect (PyObject *self, PyObject *value, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    SDL_Rect rect;
    
    if (value == Py_None)
    {
        SDL_SetClipRect (surface, NULL);
        return 0;
    }
    if (!SDLRect_FromRect (value, &rect))
        return -1;
    SDL_SetClipRect (surface, &rect);
    return 0;
}
static PyObject*
_surface_getwidth (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return PyInt_FromLong (surface->w);
}
static PyObject*
_surface_getheight (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return PyInt_FromLong (surface->h);
}

static PyObject*
_surface_getsize (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return Py_BuildValue ("(ii)", surface->w, surface->h);
}

static PyObject*
_surface_getflags (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return PyLong_FromLong ((long)surface->flags);
}

static PyObject*
_surface_getformat (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return PyPixelFormat_NewFromSDLPixelFormat (surface->format);
}

static PyObject*
_surface_getpitch (PyObject *self, void *closure)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    return PyInt_FromLong (surface->pitch);
}

static PyObject*
_surface_getpixels (PyObject *self, void *closure)
{
    PyObject *buffer;
    SDL_Surface *surface = ((PySurface*)self)->surface;

    buffer = PyBufferProxy_New (NULL, NULL, 0, PySurface_RemoveRefLock);
    if (!buffer)
        return NULL;
    if (!PySurface_AddRefLock (self, buffer))
        return NULL;
    ((PyBufferProxy*)buffer)->object = self;
    ((PyBufferProxy*)buffer)->buffer = surface->pixels;
    ((PyBufferProxy*)buffer)->length = (Py_ssize_t) surface->pitch * surface->h;

    return buffer;
}

static PyObject*
_surface_getlocked (PyObject *self, void *closure)
{
    PySurface *sf = (PySurface*) self;

    if (sf->intlocks != 0)
        Py_RETURN_TRUE;

    if (sf->locklist && PyList_Size (sf->locklist) != 0)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

/* Surface methods */
static PyObject*
_surface_update (PyObject *self, PyObject *args)
{
    PyObject *rectlist, *item;
    SDL_Rect *rects, r;
    Py_ssize_t count, i;
    
    if (!PyArg_ParseTuple (args, "O:update", &rectlist))
        return NULL;

    if (SDLRect_FromRect (rectlist, &r))
    {
        /* Single rect to update */
        Py_BEGIN_ALLOW_THREADS;
        SDL_UpdateRect (((PySurface*)self)->surface, r.x, r.y, r.w, r.h);
        Py_END_ALLOW_THREADS;
        Py_RETURN_NONE;
    }
    
    if (!PySequence_Check (rectlist))
    {
        PyErr_SetString (PyExc_TypeError,
            "argument must be a Rect or list of Rect objects");
        return NULL;
    }

    /* Sequence of rects only? */
    count = PySequence_Size (rectlist);
    rects = PyMem_New (SDL_Rect, (size_t) count);
    if (!rects)
        return NULL;

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (rectlist, i);

        if (!SDLRect_FromRect (item, &(rects[i])))
        {
            Py_XDECREF (item);
            PyMem_Free (rects);
            PyErr_SetString (PyExc_ValueError,
                "list may only contain Rect objects");
            return NULL;
        }
        Py_DECREF (item);
    }

    Py_BEGIN_ALLOW_THREADS;
    SDL_UpdateRects (((PySurface*)self)->surface, (int)count, rects);
    Py_END_ALLOW_THREADS;
    PyMem_Free (rects);

    Py_RETURN_NONE;
}

static PyObject*
_surface_flip (PyObject *self)
{
    int ret;
    
    Py_BEGIN_ALLOW_THREADS;
    ret = SDL_Flip (((PySurface*)self)->surface);
    Py_END_ALLOW_THREADS;
    
    if (ret == -1)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject*
_surface_setcolors (PyObject *self, PyObject *args)
{
    PyObject *colorlist, *item;
    Py_ssize_t count, i;
    SDL_Color *colors;
    int ret;
    
    if (!PyArg_ParseTuple (args, "O:set_colors", &colorlist))
        return NULL;

    if (!PySequence_Check (colorlist))
    {
        PyErr_SetString (PyExc_TypeError,
            "argument must be a list of Color objects");
        return NULL;
    }

    count = PySequence_Size (colorlist);
    colors = PyMem_New (SDL_Color, (size_t) count);
    if (!colors)
        return NULL;

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (colorlist, i);

        if (!PyColor_Check (item))
        {
            Py_XDECREF (item);
            PyMem_Free (colors);

            PyErr_SetString (PyExc_ValueError,
                "list may only contain Color objects");
            return NULL;
        }
        colors[i].r = ((PyColor*)item)->r;
        colors[i].g = ((PyColor*)item)->g;
        colors[i].b = ((PyColor*)item)->b;
        Py_DECREF (item);
    }
    
    ret = SDL_SetColors (((PySurface*)self)->surface, colors, 0, (int)count);
    PyMem_Free (colors);

    if (!ret)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject*
_surface_getpalette (PyObject *self)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    SDL_Palette *pal = surface->format->palette;
    SDL_Color *c;
    PyObject *tuple, *color;
    Py_ssize_t i;

    if (!pal)
        Py_RETURN_NONE;

    tuple = PyTuple_New ((Py_ssize_t) pal->ncolors);
    if (!tuple)
        return NULL;

    for (i = 0; i < pal->ncolors; i++)
    {
        c = &pal->colors[i];
        color = Py_BuildValue ("(bbb)", c->r, c->g, c->b);
        if (!color)
        {
            Py_DECREF (tuple);
            return NULL;
        }
        PyTuple_SET_ITEM (tuple, i, color);
    }
    return tuple;
}

static PyObject*
_surface_setpalette (PyObject *self, PyObject *args)
{
    PyObject *colorlist, *item;
    Py_ssize_t count, i;
    SDL_Color *palette;
    int ret, flags;

    if (!PyArg_ParseTuple (args, "Oi:set_palette", &colorlist, &flags))
        return NULL;

    if (!PySequence_Check (colorlist))
    {
        PyErr_SetString (PyExc_TypeError,
            "argument must be a list of Color objects");
        return NULL;
    }

    count = PySequence_Size (colorlist);
    palette = PyMem_New (SDL_Color, (size_t) count);
    if (!palette)
        return NULL;

    for (i = 0; i < count; i++)
    {
        item = PySequence_ITEM (colorlist, i);

        if (!PyColor_Check (item))
        {
            Py_XDECREF (item);
            PyMem_Free (palette);

            PyErr_SetString (PyExc_ValueError,
                "list may only contain Color objects");
            return NULL;
        }
        palette[i].r = ((PyColor*)item)->r;
        palette[i].g = ((PyColor*)item)->g;
        palette[i].b = ((PyColor*)item)->b;
        Py_DECREF (item);
    }
    
    ret = SDL_SetPalette (((PySurface*)self)->surface, (int)flags, palette, 0,
        (int)count);
    PyMem_Free (palette);

    if (!ret)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject*
_surface_lock (PyObject *self)
{
    if (SDL_LockSurface (((PySurface*)self)->surface) == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    ((PySurface*)self)->intlocks++;
    Py_RETURN_NONE;
}

static PyObject*
_surface_unlock (PyObject *self)
{
    if (((PySurface*)self)->intlocks == 0)
        Py_RETURN_NONE;

    SDL_UnlockSurface (((PySurface*)self)->surface);
    ((PySurface*)self)->intlocks--;
    Py_RETURN_NONE;
}

static PyObject*
_surface_getcolorkey (PyObject *self)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;
    Uint8 r, g, b, a;

    if (!(surface->flags & SDL_SRCCOLORKEY))
        Py_RETURN_NONE;

    SDL_GetRGBA (surface->format->colorkey, surface->format, &r, &g, &b, &a);
    return Py_BuildValue ("(bbbb)", r, g, b, a);
}

static PyObject*
_surface_setcolorkey (PyObject *self, PyObject *args)
{
    Uint32 flags, key;
    PyObject *colorkey;

    if (!PyArg_ParseTuple (args, "lO:set_colorkey", &flags, &colorkey))
        return NULL;

    if (PyColor_Check (colorkey))
        key = (Uint32) PyColor_AsNumber (colorkey);
    else if (!Uint32FromObj (colorkey, &key))
        return NULL;

    RGB2FORMAT (key, ((PySurface*)self)->surface->format);
    if (SDL_SetColorKey (((PySurface*)self)->surface, flags, key) == -1)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject*
_surface_getalpha (PyObject *self)
{
    SDL_Surface *surface = ((PySurface*)self)->surface;

    if (surface->flags & SDL_SRCALPHA)
        return PyInt_FromLong (surface->format->alpha);
    Py_RETURN_NONE;
}

static PyObject*
_surface_setalpha (PyObject *self, PyObject *args)
{
    Uint32 flags;
    Uint8 alpha;

    if (!PyArg_ParseTuple (args, "lb:set_alpha", &flags, &alpha))
        return NULL;

    if (SDL_SetAlpha (((PySurface*)self)->surface, flags, alpha) == -1)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject*
_surface_convert (PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pxfmt = NULL, *sf;
    Uint32 flags = 0;
    SDL_Surface *surface;
    SDL_PixelFormat *fmt;
    
    static char *keys[] = { "format", "flags", NULL };
    if (!PyArg_ParseTupleAndKeywords (args, kwds, "|Ol;convert", keys, &pxfmt,
        &flags))
        return NULL;

    if (pxfmt && !PyPixelFormat_Check (pxfmt))
    {
        PyErr_SetString (PyExc_TypeError, "pxfmt must be a PixelFormat");
        return NULL;
    }
    
    if (!pxfmt && flags == 0)
    {
        surface = SDL_DisplayFormat (((PySurface*)self)->surface);
    }
    else
    {
        if (pxfmt)
            fmt = ((PyPixelFormat*)pxfmt)->format;
        else
            fmt = ((PySurface*)self)->surface->format;

        surface = SDL_ConvertSurface (((PySurface*)self)->surface, fmt, flags);
    }

    if (!surface)
        return NULL;
    
    sf = PySurface_NewFromSDLSurface (surface);
    if (!sf)
    {
        SDL_FreeSurface (surface);
        return NULL;
    }
    return sf;
}

static PyObject*
_surface_getat (PyObject *self, PyObject *args)
{
    int x, y;
    SDL_Surface *surface = PySurface_AsSurface (self);
    SDL_PixelFormat *fmt = surface->format;
    Uint8 r, g, b, a;
    Uint32 value;

    if (!PyArg_ParseTuple (args, "ii", &x, &y))
        return NULL;
    
    if (x < 0 || x > surface->w || y < 0 || y >= surface->h)
    {
        PyErr_SetString (PyExc_IndexError, "pixel index out of range");
        return NULL;
    }
    
    if (fmt->BytesPerPixel < 1 || fmt->BytesPerPixel > 4)
    {
        PyErr_SetString (PyExc_TypeError, "invalid bit depth for surface");
        return NULL;
    }
    if (SDL_LockSurface (surface) == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    GET_PIXEL_AT (value, surface, fmt->BytesPerPixel, x, y);
    SDL_UnlockSurface (surface);
    SDL_GetRGBA (value, fmt, &r, &g, &b, &a);
    return Py_BuildValue ("(bbbb)", r, g, b, a);
}

static PyObject*
_surface_setat (PyObject *self, PyObject *args)
{
    int x, y;
    SDL_Surface *surface = PySurface_AsSurface (self);
    SDL_PixelFormat *fmt = surface->format;
    PyObject *color;
    Uint32 value;

    if (!PyArg_ParseTuple (args, "iiO", &x, &y, &color))
        return NULL;
    
    if (x < 0 || x > surface->w || y < 0 || y >= surface->h)
    {
        PyErr_SetString (PyExc_IndexError, "pixel index out of range");
        return NULL;
    }

    if (!Uint32FromObj (color, &value))
        return NULL;
    ARGB2FORMAT (value, surface->format);
    
    if (fmt->BytesPerPixel < 1 || fmt->BytesPerPixel > 4)
    {
        PyErr_SetString (PyExc_TypeError, "invalid bit depth for surface");
        return NULL;
    }
    if (SDL_LockSurface (surface) == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    SET_PIXEL_AT (surface, fmt, x, y, value);
    SDL_UnlockSurface (surface);
    Py_RETURN_NONE;
}

static PyObject*
_surface_clone (PyObject *self)
{
    return PySurface_Clone (self);
}

static PyObject*
_surface_blit (PyObject *self, PyObject *args, PyObject *kwds)
{
    SDL_Surface *src, *dst;
    SDL_Rect srcrect, dstrect;
    PyObject *srcsf, *srcr = NULL, *dstr = NULL;
    int blitargs = 0;

    static char *keys[] = { "surface", "dstrect", "srcrect", "blendargs",
                            NULL };
    if (!PyArg_ParseTupleAndKeywords (args, kwds, "O|OOi:blit", keys, &srcsf,
            &dstr, &srcr, &blitargs))
        return NULL;
        
    if (!PySurface_Check (srcsf))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return NULL;
    }
    if (srcr && !SDLRect_FromRect (srcr, &srcrect))
    {
        PyErr_Clear();
        PyErr_SetString (PyExc_TypeError, "srcrect must be a Rect or FRect");
        return NULL;
    }
    if (dstr && !SDLRect_FromRect (dstr, &dstrect))
    {
        PyErr_Clear();
        PyErr_SetString (PyExc_TypeError, "dstrect must be a Rect or FRect");
        return NULL;
    }

    src = ((PySurface*)srcsf)->surface;
    dst = ((PySurface*)self)->surface;
    
    if (!srcr)
    {
        srcrect.x = srcrect.y = 0;
        srcrect.w = src->w;
        srcrect.h = src->h;
    }

    if (!dstr)
    {
        dstrect.x = dstrect.y = 0;
        dstrect.w = dst->w;
        dstrect.h = dst->h;
    }

    if (dst->flags & SDL_OPENGL &&
        !(dst->flags & (SDL_OPENGLBLIT & ~SDL_OPENGL)))
    {
        PyErr_SetString (PyExc_PyGameError,
            "cannot blit to OPENGL Surfaces (OPENGLBLIT is ok)");
        return NULL;
    }

    /* TODO: Check if all blit combinations work. */
    if (blitargs != 0)
    {
        if (!pyg_software_blit (src, &srcrect, dst, &dstrect, blitargs))
        {
            PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
            return NULL;
        }
    }
    else if (SDL_BlitSurface (src, &srcrect, dst, &dstrect) == -1)
        Py_RETURN_NONE;
    return PyRect_New (dstrect.x, dstrect.y, dstrect.w, dstrect.h);
}

static PyObject*
_surface_fill (PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *color, *dstrect = NULL;
    SDL_Rect rect;
    SDL_Surface *surface;
    Uint32 col;
    int ret, blendargs = -1;
    
    static char *keys[] = { "color", "rect", "blendargs", NULL };
    if (!PyArg_ParseTupleAndKeywords (args, kwds, "O|Oi", keys, &color,
            &dstrect, &blendargs))
        return NULL;
        
    if (!PyColor_Check (color))
    {
        PyErr_SetString (PyExc_TypeError, "color must be a Color");
        return NULL;
    }
    if (dstrect && !SDLRect_FromRect (dstrect, &rect))
    {
        PyErr_SetString (PyExc_TypeError, "rect must be a Rect");
        return NULL;
    }
    
    surface = ((PySurface*)self)->surface;
    if (!dstrect)
    {
        rect.x = rect.y = 0;
        rect.w = surface->w;
        rect.h = surface->h;
    }
    
    col = (Uint32) PyColor_AsNumber (color);
    ARGB2FORMAT (col, surface->format);

    Py_BEGIN_ALLOW_THREADS;
    if (blendargs == -1)
        ret = SDL_FillRect (surface, &rect, col);
    else
        ret =  pyg_surface_fill_blend (surface, &rect, col, blendargs);
    Py_END_ALLOW_THREADS;
        
    if (ret == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_surface_save (PyObject *self, PyObject *args)
{
    SDL_Surface *surface;
    PyObject *file;
    char *filename, *type = NULL;
    int retval;

    if (!PyArg_ParseTuple (args, "O|s", &file, &type))
        return NULL;

    surface = ((PySurface*)self)->surface;

    if (PyString_Check (file) || PyUnicode_Check (file))
    {
        filename = PyString_AsString (file);
        Py_BEGIN_ALLOW_THREADS;
        retval = pyg_surface_save (surface, filename, type);
        Py_END_ALLOW_THREADS;
    }
    else if (PyFile_Check (file))
    {
        SDL_RWops* rw;
        
        /* If the type is NULL, we assume TGA saving. */
        if (!type)
            type = "TGA";

        if (!(rw = RWopsFromPython (file)))
            return NULL;
        Py_BEGIN_ALLOW_THREADS;
        retval = pyg_surface_save_rw (surface, rw, type);
        Py_END_ALLOW_THREADS;
    }
    else
    {
        PyErr_SetString (PyExc_TypeError,
            "file must be file-like object or a string");
        return NULL;
    }
    
    if (!retval)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

/* C API */
static void
_release_c_lock (void *ptr)
{
    SurfaceLock* c_lock = (SurfaceLock*) ptr;

    SDL_UnlockSurface (((PySurface*)c_lock->surface)->surface);
    Py_XDECREF (c_lock->surface);
    Py_XDECREF (c_lock->lockobj);
    PyMem_Free (c_lock);
}

PyObject*
PySurface_New (int w, int h)
{
    SDL_Surface *sf, *video;
    PyObject *surface;

    ASSERT_VIDEO_INIT (NULL);
    video = SDL_GetVideoSurface ();
    if (!video)
    {
        PyErr_SetString (PyExc_PyGameError, "display surface not set");
        return NULL;
    }

    sf = SDL_CreateRGBSurface (video->flags, w, h, video->format->BitsPerPixel,
        video->format->Rmask, video->format->Gmask, video->format->Bmask,
        video->format->Amask);
    if (!sf)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }

    surface = (PyObject*) PySurface_Type.tp_new (&PySurface_Type, NULL, NULL);
    if (!surface)
    {
        SDL_FreeSurface (sf);
        return NULL;
    }
    ((PySurface*)surface)->surface = sf;
    return surface;
}

PyObject*
PySurface_NewFromSDLSurface (SDL_Surface *sf)
{
    PySurface *surface;
    if (!sf)
        return NULL;
    surface = (PySurface*) PySurface_Type.tp_new (&PySurface_Type, NULL, NULL);
    if (!surface)
        return NULL;

    surface->surface = sf;
    return (PyObject*) surface;
}

int
PySurface_AddRefLock (PyObject *surface, PyObject *lock)
{
    PySurface *sf = (PySurface*)surface;
    PyObject *wkref;

    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return 0;
    }
    if (!lock)
    {
        PyErr_SetString (PyExc_TypeError, "lock must not be NULL");
        return 0;
    }

    if (!sf->locklist)
    {
        sf->locklist = PyList_New (0);
        if (!sf->locklist)
            return 0;
    }

    if (SDL_LockSurface (sf->surface) == -1)
        return 0;

    wkref = PyWeakref_NewRef (lock, NULL);
    if (!wkref)
        return 0;

    if (PyList_Append (sf->locklist, wkref) == -1)
    {
        SDL_UnlockSurface (sf->surface);
        Py_DECREF (wkref);
        return 0;
    }

    return 1;
}

int
PySurface_RemoveRefLock (PyObject *surface, PyObject *lock)
{
    PySurface *sf = (PySurface*)surface;
    PyObject *ref, *item;
    Py_ssize_t size;
    int found = 0, noerror = 1;

    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return 0;
    }

    if (!sf->locklist)
    {
        PyErr_SetString (PyExc_ValueError, "no locks are hold by the object");
        return 0;
    }

    size = PyList_Size (sf->locklist);
    if (size == 0)
    {
        PyErr_SetString (PyExc_ValueError, "no locks are hold by the object");
        return 0;
    }
    
    while (--size >= 0)
    {
        ref = PyList_GET_ITEM (sf->locklist, size);
        item = PyWeakref_GET_OBJECT (ref);
        if (item == lock)
        {
            if (PySequence_DelItem (sf->locklist, size) == -1)
                return 0;
            found++;
        }
        else if (item == Py_None)
        {
            /* Clear dead references */
            if (PySequence_DelItem (sf->locklist, size) != -1)
                found++;
            else
                noerror = 0;
        }
    }
    if (!found)
        return noerror;

    /* Release all locks on the surface */
    while (found > 0)
    {
        SDL_UnlockSurface (sf->surface);
        found--;
    }
    return noerror;
}

PyObject*
PySurface_AcquireLockObj (PyObject *surface, PyObject *lock)
{
    PyObject *cobj;
    SurfaceLock *c_lock;

    if (!PySurface_Check (surface))
    {
        PyErr_SetString (PyExc_TypeError, "surface must be a Surface");
        return 0;
    }

    c_lock = PyMem_New (SurfaceLock, 1);
    if (!c_lock)
        return NULL;

    Py_INCREF (surface);
    Py_XINCREF (lock);
    c_lock->surface = surface;
    c_lock->lockobj = lock;

    if (SDL_LockSurface (((PySurface*)surface)->surface) == -1)
    {
        PyMem_Free (c_lock);
        Py_DECREF (surface);
        Py_XDECREF (lock);
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }

    cobj = PyCObject_FromVoidPtr (c_lock, _release_c_lock);
    if (!cobj)
    {
        SDL_UnlockSurface (((PySurface*)surface)->surface);
        PyMem_Free (c_lock);
        Py_DECREF (surface);
        Py_XDECREF (lock);
        return NULL;
    }

    return cobj;
}

PyObject*
PySurface_Clone (PyObject *source)
{
    PyObject *surfobj;
    SDL_Surface *surface, *newsurface;

    if (!PySurface_Check (source))
    {
        PyErr_SetString (PyExc_TypeError, "source must be a Surface");
        return NULL;
    }

    surface = ((PySurface*)source)->surface;

    /* TODO: does this really copy anything? */
    newsurface = SDL_ConvertSurface (surface, surface->format, surface->flags);
    if (!newsurface)
        return NULL;

    surfobj = PySurface_NewFromSDLSurface (newsurface);
    if (!surfobj)
    {
        SDL_FreeSurface (newsurface);
        return NULL;
    }
    return surfobj;
}

void
surface_export_capi (void **capi)
{
    capi[PYGAME_SDLSURFACE_FIRSTSLOT] = &PySurface_Type;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+1] = PySurface_New;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+2] = PySurface_NewFromSDLSurface;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+3] = PySurface_Clone;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+4] = PySurface_AddRefLock;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+5] = PySurface_RemoveRefLock;
    capi[PYGAME_SDLSURFACE_FIRSTSLOT+6] = PySurface_AcquireLockObj;
}
