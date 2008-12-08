/*
  pygame - Python Game Library
  Copyright (C) 2000-2001 Pete Shinners, 2008 Marcus von Appen

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
#define PYGAME_SDLTTF_INTERNAL

#include "ttfmod.h"
#include "pgttf.h"
#include "pgsdl.h"

static void _quit (void);

static PyObject* _ttf_init (PyObject *self);
static PyObject* _ttf_wasinit (PyObject *self);
static PyObject* _ttf_quit (PyObject *self);
static PyObject* _ttf_getcompiledversion (PyObject *self);
static PyObject* _ttf_getversion (PyObject *self);
static PyObject* _ttf_geterror (PyObject *self);
static PyObject* _ttf_byteswappedunicode (PyObject *self, PyObject *args);

static PyMethodDef _ttf_methods[] = {
    { "init", (PyCFunction) _ttf_init, METH_NOARGS, "" },
    { "was_init", (PyCFunction) _ttf_wasinit, METH_NOARGS, "" },
    { "quit", (PyCFunction) _ttf_quit, METH_NOARGS, "" },
    { "get_compiled_version", (PyCFunction) _ttf_getcompiledversion,
      METH_NOARGS, "" },
    { "get_version", (PyCFunction) _ttf_getversion, METH_NOARGS, "" },
    { "get_error", (PyCFunction) _ttf_geterror, METH_NOARGS, "" },
    { "set_byte_swapped_unicode", _ttf_byteswappedunicode, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL },
};

static void
_quit (void)
{
    if (TTF_WasInit ())
        TTF_Quit ();
}

static PyObject*
_ttf_init (PyObject *self)
{
    if (TTF_WasInit ())
        Py_RETURN_NONE;
    if (TTF_Init () == -1)
    {
        PyErr_SetString (PyExc_PyGameError, TTF_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_ttf_wasinit (PyObject *self)
{
    if (TTF_WasInit ())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject*
_ttf_quit (PyObject *self)
{
    _quit ();
    Py_RETURN_NONE;
}

static PyObject*
_ttf_getcompiledversion (PyObject *self)
{
    SDL_version compiled;
    TTF_VERSION (&compiled);
    return Py_BuildValue ("(iii)", compiled.major, compiled.minor,
        compiled.patch);
}

static PyObject*
_ttf_getversion (PyObject *self)
{
    const SDL_version *linked = TTF_Linked_Version ();
    return Py_BuildValue ("(iii)", linked->major, linked->minor, linked->patch);
}

static PyObject*
_ttf_geterror (PyObject *self)
{
    char *err = TTF_GetError ();
    if (!err)
        Py_RETURN_NONE;
    return Text_FromUTF8 (err);
}

static PyObject*
_ttf_byteswappedunicode (PyObject *self, PyObject *args)
{
    PyObject *bool;
    int istrue;

    if (!PyArg_ParseTuple (args, "O:set_byte_swapped_unicode", &bool))
        return NULL;

    istrue = PyObject_IsTrue (bool);
    if (istrue == -1)
        return NULL;
    TTF_ByteSwappedUNICODE (istrue);
    Py_RETURN_NONE;
}

#if PY_VERSION_HEX >= 0x03000000
PyMODINIT_FUNC PyInit_base (void)
#else
PyMODINIT_FUNC initbase (void)
#endif
{
    PyObject *mod = NULL;
    PyObject *c_api_obj;
    static void *c_api[PYGAME_SDLTTF_SLOTS];
    
#if PY_VERSION_HEX >= 0x03000000
    static struct PyModuleDef _module = {
        PyModuleDef_HEAD_INIT,
        "base",
        "",
        -1,
        _ttf_methods,
        NULL, NULL, NULL, NULL
    };
#endif

    if (PyType_Ready (&PyFont_Type) < 0)
        goto fail;

    Py_INCREF (&PyFont_Type);

#if PY_VERSION_HEX < 0x03000000
    mod = Py_InitModule3 ("base", _ttf_methods, "");
#else
    mod = PyModule_Create (&_module);
#endif
    if (!mod)
        goto fail;
    
    PyModule_AddObject (mod, "Font", (PyObject *) &PyFont_Type);

    /* Export C API */
    font_export_capi (c_api);

    c_api_obj = PyCObject_FromVoidPtr ((void *) c_api, NULL);
    if (c_api_obj)
        PyModule_AddObject (mod, PYGAME_SDLTTF_ENTRY, c_api_obj);    

    if (import_pygame2_base () < 0)
        goto fail;
    if (import_pygame2_sdl_base () < 0)
        goto fail;
    if (import_pygame2_sdl_video () < 0)
        goto fail;
    RegisterQuitCallback (_quit);
    MODINIT_RETURN(mod);
fail:
    Py_XDECREF (mod);
    MODINIT_RETURN (NULL);
}
