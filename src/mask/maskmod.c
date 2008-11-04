/*
  Copyright (C) 2002-2007 Ulf Ekstrom except for the bitcount function.
  This wrapper code was originally written by Danny van Bruggen(?) for
  the SCAM library, it was then converted by Ulf Ekstrom to wrap the
  bitmask library, a spinoff from SCAM.

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

/* a couple of print debugging helpers */
/*
  #define CALLLOG2(x,y) fprintf(stderr, (x), (y));
  #define CALLLOG(x) fprintf(stderr, (x));
*/

#define PYGAME_MASKMOD_INTERNAL

#include <stdlib.h>
#include "pgbase.h"
#include "maskmod.h"
#include "pgmask.h"
#include "mask_doc.h"

#ifndef HAVE_PYGAME_SDL_VIDEO
/* Used, if there is no sdl.video module available. */

static PyMethodDef _mask_methods[] =
{
    { NULL, NULL, 0, NULL }
};

#endif /* !HAVE_PYGAME_SDL_VIDEO */


/* Only build those, if the sdl.video module is available. */

#ifdef HAVE_PYGAME_SDL_VIDEO

#include "pgsdl.h"
#include "surface.h"

static void
_bitmask_threshold (bitmask_t *m, SDL_Surface *surf, SDL_Surface *surf2, 
    Uint32 color, Uint32 threshold);

static PyObject* _mask_fromsurface (PyObject* self, PyObject* args);
static PyObject* _mask_fromthreshold (PyObject* self, PyObject* args);

static PyMethodDef _mask_methods[] =
{
    { "from_surface", _mask_fromsurface, METH_VARARGS, DOC_MASK_FROM_SURFACE },
    { "from_threshold", _mask_fromthreshold, METH_VARARGS, 
      DOC_MASK_FROM_THRESHOLD },
    { NULL, NULL, 0, NULL }
};

static PyObject*
_mask_fromsurface (PyObject* self, PyObject* args)
{
    bitmask_t *mask;
    SDL_Surface* surf;

    PyObject* surfobj, *lock;
    PyMask *maskobj;

    int x, y, threshold, ashift, aloss, usethresh;
    Uint8 *pixels;

    SDL_PixelFormat *format;
    Uint32 color, amask;
    Uint8 a;

    /* set threshold as 127 default argument. */
    threshold = 127;

    /* get the surface from the passed in arguments. 
     *   surface, threshold
     */

    if (!PyArg_ParseTuple (args, "O!|i", &PySurface_Type, &surfobj,
            &threshold))
        return NULL;

    surf = PySurface_AsSurface (surfobj);

    /* get the size from the surface, and create the mask. */
    mask = bitmask_create (surf->w, surf->h);

    if (!mask)
    {
        PyErr_SetString (PyExc_MemoryError, "memory allocation failed");
        return NULL;
    }

    /* lock the surface, release the GIL. */
    lock = PySurface_AcquireLockObj (surfobj, self);
    if (!lock)
        return NULL;

    Py_BEGIN_ALLOW_THREADS;

    pixels = (Uint8 *) surf->pixels;
    format = surf->format;
    amask = format->Amask;
    ashift = format->Ashift;
    aloss = format->Aloss;
    usethresh = !(surf->flags & SDL_SRCCOLORKEY);

    for (y=0; y < surf->h; y++)
    {
        pixels = (Uint8 *) surf->pixels + y*surf->pitch;
        for (x=0; x < surf->w; x++)
        {
            /* Get the color.*/
            GET_PIXEL (color, format->BytesPerPixel, pixels);
            pixels += format->BytesPerPixel;

            if (usethresh)
            {
                a = ((color & amask) >> ashift) << aloss;
                /* no colorkey, so we check the threshold of the alpha */
                if (a > threshold)
                {
                    bitmask_setbit(mask, x, y);
                }
            }
            else
            {
                /*  test against the colour key. */
                if (format->colorkey != color)
                {
                    bitmask_setbit(mask, x, y);
                }
            }
        }
    }

    Py_END_ALLOW_THREADS;

    /* unlock the surface, release the GIL.
     */
    Py_DECREF (lock);

    /*create the new python object from mask*/        
    maskobj = (PyMask*)PyMask_Type.tp_new (&PyMask_Type, NULL, NULL);
    if (maskobj)
        maskobj->mask = mask;
    else
        bitmask_free (mask);

    return (PyObject*)maskobj;
}

static PyObject*
_mask_fromthreshold (PyObject* self, PyObject* args)
{
    PyObject *surfobj, *surfobj2, *lock1, *lock2 = NULL;
    PyMask *maskobj;
    bitmask_t* m;
    SDL_Surface* surf, *surf2;
    int bpp;
    PyObject *rgba_obj_color, *rgba_obj_threshold;
    Uint32 color;
    Uint32 color_threshold;

    surf2 = surf = NULL;
    surfobj2 = NULL;
    rgba_obj_threshold = NULL;

    if (!PyArg_ParseTuple (args, "O!O|OO!", &PySurface_Type, &surfobj,
            &rgba_obj_color,  &rgba_obj_threshold,
            &PySurface_Type, &surfobj2))
        return NULL;

    surf = PySurface_AsSurface (surfobj);
    if (surfobj2)
        surf2 = PySurface_AsSurface (surfobj2);

    if (!ColorFromObj (rgba_obj_color, surf->format, &color))
        return NULL;

    if (rgba_obj_threshold)
    {
        if (!ColorFromObj (rgba_obj_threshold, surf->format, &color_threshold))
            return NULL;
    }
    else
    {
        color_threshold = SDL_MapRGBA (surf->format, 0, 0, 0, 255);
    }

    bpp = surf->format->BytesPerPixel;
    m = bitmask_create(surf->w, surf->h);
    if (!m)
    {
        PyErr_SetString (PyExc_MemoryError, "memory allocation failed");
        return NULL;
    }
    
    lock1 = PySurface_AcquireLockObj (surfobj, self);
    if (!lock1)
    {
        bitmask_free (m);
        return NULL;
    }

    if (surfobj2)
    {
        lock2 = PySurface_AcquireLockObj (surfobj2, self);
        if (!lock2)
        {
            Py_DECREF (lock1);
            bitmask_free (m);
            return NULL;
        }
    }
    
    Py_BEGIN_ALLOW_THREADS;
    _bitmask_threshold (m, surf, surf2, color,  color_threshold);
    Py_END_ALLOW_THREADS;

    Py_DECREF (lock1);
    Py_XDECREF (lock2);

    maskobj = (PyMask*)PyMask_Type.tp_new (&PyMask_Type, NULL, NULL);
    if(maskobj)
        maskobj->mask = m;
    else
        bitmask_free (m);

    return (PyObject*)maskobj;
}

static void
_bitmask_threshold (bitmask_t *m, SDL_Surface *surf, SDL_Surface *surf2, 
    Uint32 color,  Uint32 threshold)
{
    int x, y, rshift, gshift, bshift, rshift2, gshift2, bshift2;
    int rloss, gloss, bloss, rloss2, gloss2, bloss2;
    Uint8 *pixels, *pixels2;
    SDL_PixelFormat *format, *format2;
    Uint32 the_color, the_color2, rmask, gmask, bmask, rmask2, gmask2, bmask2;
    Uint8 *pix;
    Uint8 r, g, b, a;
    Uint8 tr, tg, tb, ta;

    pixels = (Uint8 *) surf->pixels;
    format = surf->format;
    rmask = format->Rmask;
    gmask = format->Gmask;
    bmask = format->Bmask;
    rshift = format->Rshift;
    gshift = format->Gshift;
    bshift = format->Bshift;
    rloss = format->Rloss;
    gloss = format->Gloss;
    bloss = format->Bloss;

    if (surf2)
    {
        format2 = surf2->format;
        rmask2 = format2->Rmask;
        gmask2 = format2->Gmask;
        bmask2 = format2->Bmask;
        rshift2 = format2->Rshift;
        gshift2 = format2->Gshift;
        bshift2 = format2->Bshift;
        rloss2 = format2->Rloss;
        gloss2 = format2->Gloss;
        bloss2 = format2->Bloss;
        pixels2 = (Uint8 *) surf2->pixels;
    }
    else
    {
        /* make gcc stop complaining */
        rmask2 = gmask2 = bmask2 = 0;
        rshift2 = gshift2 = bshift2 = 0;
        rloss2 = gloss2 = bloss2 = 0;
        format2 = NULL;
        pixels2 = NULL;
    }

    SDL_GetRGBA (color, format, &r, &g, &b, &a);
    SDL_GetRGBA (threshold, format, &tr, &tg, &tb, &ta);

    for (y=0; y < surf->h; y++)
    {
        pixels = (Uint8 *) surf->pixels + y*surf->pitch;
        if (surf2)
        {
            pixels2 = (Uint8 *) surf2->pixels + y*surf2->pitch;
        }

        for (x=0; x < surf->w; x++)
        {
            /* the_color = surf->get_at(x,y) */
            switch (format->BytesPerPixel)
            {
            case 1:
                the_color = (Uint32)*((Uint8 *) pixels);
                pixels++;
                break;
            case 2:
                the_color = (Uint32)*((Uint16 *) pixels);
                pixels += 2;
                break;
            case 3:
                pix = ((Uint8 *) pixels);
                pixels += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                the_color = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                the_color = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                break;
            default:                  /* case 4: */
                the_color = *((Uint32 *) pixels);
                pixels += 4;
                break;
            }

            if (surf2)
            {
                switch (format2->BytesPerPixel)
                {
                case 1:
                    the_color2 = (Uint32)*((Uint8 *) pixels2);
                    pixels2++;
                    break;
                case 2:
                    the_color2 = (Uint32)*((Uint16 *) pixels2);
                    pixels2 += 2;
                    break;
                case 3:
                    pix = ((Uint8 *) pixels2);
                    pixels2 += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                    the_color2 = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                    the_color2 = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                    break;
                default:                  /* case 4: */
                    the_color2 = *((Uint32 *) pixels2);
                    pixels2 += 4;
                    break;
                }              
                if ((abs((((the_color2 & rmask2) >> rshift2) << rloss2) -
 (((the_color & rmask) >> rshift) << rloss)) < tr) & 
                    (abs((((the_color2 & gmask2) >> gshift2) << gloss2) - (((the_color & gmask) >> gshift) << gloss)) < tg) & 
                    (abs((((the_color2 & bmask2) >> bshift2) << bloss2) - (((the_color & bmask) >> bshift) << bloss)) < tb))
                {
                    /* this pixel is within the threshold of othersurface. */
                    bitmask_setbit(m, x, y);
                }
            }
            else if ((abs((((the_color & rmask) >> rshift) << rloss) - r) < tr)&
                (abs((((the_color & gmask) >> gshift) << gloss) - g) < tg) & 
                (abs((((the_color & bmask) >> bshift) << bloss) - b) < tb))
            {
                /* this pixel is within the threshold of the color. */
                bitmask_setbit(m, x, y);
            }
        }
    }
}

#endif /* HAVE_PYGAME_SDL_VIDEO */

#if PY_VERSION_HEX >= 0x03000000
PyMODINIT_FUNC PyInit_mask (void)
#else
PyMODINIT_FUNC initmask (void)
#endif
{
    PyObject *mod = NULL;
    PyObject *c_api_obj;
    static void *c_api[PYGAME_MASK_SLOTS];

#if PY_VERSION_HEX >= 0x03000000
    static struct PyModuleDef _maskmodule = {
        PyModuleDef_HEAD_INIT, "mask", DOC_MASK, -1, _mask_methods,
        NULL, NULL, NULL, NULL
    };
#endif

    /* Complete types */
    if (PyType_Ready (&PyMask_Type) < 0)
        goto fail;
    
    Py_INCREF (&PyMask_Type);

#if PY_VERSION_HEX < 0x03000000
    mod = Py_InitModule3 ("mask", _mask_methods, DOC_MASK);
#else
    mod = PyModule_Create (&_maskmodule);
#endif
    if (!mod)
        goto fail;

    PyModule_AddObject (mod, "Mask", (PyObject *) &PyMask_Type);
    
    mask_export_capi (c_api);

    c_api_obj = PyCObject_FromVoidPtr ((void *) c_api, NULL);
    if (c_api_obj)
        PyModule_AddObject (mod, PYGAME_MASK_ENTRY, c_api_obj);

    import_pygame2_base ();

#ifdef HAVE_PYGAME_SDL_VIDEO
    if (import_pygame2_sdl_base () < 0)
        goto fail;
    if (import_pygame2_sdl_video () < 0)
        goto fail;
#endif /* HAVE_PYGAME_SDL_VIDEO */

    MODINIT_RETURN(mod);

fail:
    Py_XDECREF (mod);
    MODINIT_RETURN (NULL);
}
