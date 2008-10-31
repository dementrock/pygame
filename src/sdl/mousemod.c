/*
  pygame - Python Game Library
  Copyright (C) 2000-2001 Pete Shinners

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
#define PYGAME_SDLMOUSE_INTERNAL

#include "mousemod.h"
#include "pgsdl.h"

static PyObject* _sdl_warpmouse (PyObject *self, PyObject *args);
static PyObject* _sdl_getmousepos (PyObject *self);
static PyObject* _sdl_getrelativepos (PyObject *self);
static PyObject* _sdl_getmousestate (PyObject *self);
static PyObject* _sdl_getrelativemousestate (PyObject *self);
static PyObject* _sdl_setvisible (PyObject *self, PyObject *args);
static PyObject* _sdl_setcursor (PyObject *self, PyObject *args);

static PyMethodDef _mouse_methods[] = {
    { "warp", _sdl_warpmouse, METH_VARARGS, "" },
    { "set_position", _sdl_warpmouse, METH_VARARGS, "" },
    { "get_position", (PyCFunction)_sdl_getmousepos, METH_NOARGS, "" },
    { "get_rel_position", (PyCFunction)_sdl_getrelativepos, METH_NOARGS, "" },
    { "get_state", (PyCFunction)_sdl_getmousestate, METH_NOARGS, "" },
    { "get_rel_state", (PyCFunction)_sdl_getrelativemousestate, METH_NOARGS,
      "" },
    { "set_visible", _sdl_setvisible, METH_VARARGS, "" },
    { "show_cursor", _sdl_setvisible, METH_VARARGS, "" },
    { "set_cursor", _sdl_setcursor, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

static PyObject*
_sdl_warpmouse (PyObject *self, PyObject *args)
{
    Uint16 x, y;
    
    ASSERT_VIDEO_SURFACE_SET(NULL);

    if (!PyArg_ParseTuple (args, "ii:warp", &x, &y))
        return NULL;
    SDL_WarpMouse (x, y);
    Py_RETURN_NONE;
}

static PyObject*
_sdl_getmousepos (PyObject *self)
{
    int x, y;

    ASSERT_VIDEO_SURFACE_SET(NULL);

    SDL_GetMouseState (&x, &y);
    return Py_BuildValue ("(ii)", x, y);
}

static PyObject*
_sdl_getrelativepos (PyObject *self)
{
    int x, y;

    ASSERT_VIDEO_SURFACE_SET(NULL);

    SDL_GetRelativeMouseState (&x, &y);
    return Py_BuildValue ("(ii)", x, y);
}

static PyObject*
_sdl_getmousestate (PyObject *self)
{
    Uint8 buttons;
    int x, y;

    ASSERT_VIDEO_SURFACE_SET(NULL);

    buttons = SDL_GetMouseState (&x, &y);
    return Py_BuildValue ("(iii)", buttons, x, y);
}

static PyObject*
_sdl_getrelativemousestate (PyObject *self)
{
    Uint8 buttons;
    int x, y;

    ASSERT_VIDEO_SURFACE_SET(NULL);

    buttons = SDL_GetRelativeMouseState (&x, &y);
    return Py_BuildValue ("(iii)", buttons, x, y);
}

static PyObject*
_sdl_setvisible (PyObject *self, PyObject *args)
{
    PyObject *val;
    int state;

    ASSERT_VIDEO_SURFACE_SET(NULL);

    if (!PyArg_ParseTuple (args, "O:set_visible", &val))
        return NULL;

    if (PyBool_Check (val))
    {
        if (val == Py_True)
            state = SDL_ShowCursor (SDL_ENABLE);
        else
            state = SDL_ShowCursor (SDL_DISABLE);
    }
    else if (IntFromObj (val, &state))
        state = SDL_ShowCursor (state);
    else
    {
        PyErr_SetString (PyExc_TypeError, "argument must be a bool");
        return NULL;
    }

    return PyInt_FromLong (state);
}

static PyObject*
_sdl_setcursor (PyObject *self, PyObject *args)
{
    PyObject *cursor;
    SDL_Cursor *c;
    
    ASSERT_VIDEO_SURFACE_SET(NULL);
    if (!PyArg_ParseTuple (args, "O:set_cursor", &cursor))
        return NULL;
    if (!PyCursor_Check (cursor))
    {
        PyErr_SetString (PyExc_TypeError, "argument must be a Cursor");
        return NULL;
    }
    
    c = ((PyCursor*)cursor)->cursor;
    SDL_SetCursor (c);
    Py_RETURN_NONE;
}

#if PY_VERSION_HEX >= 0x03000000
PyMODINIT_FUNC PyInit_mouse (void)
#else
PyMODINIT_FUNC initmouse (void)
#endif
{
    PyObject *mod = NULL, *cursors;
    PyObject *c_api_obj;
    static void *c_api[PYGAME_SDLMOUSE_SLOTS];
    
#if PY_VERSION_HEX >= 0x03000000
    static struct PyModuleDef _module = {
        PyModuleDef_HEAD_INIT,
        "mouse",
        "",
        -1,
        _mouse_methods,
        NULL, NULL, NULL, NULL
    };
#endif

    /* Complete types */
    PyCursor_Type.tp_new = &PyType_GenericNew;
    if (PyType_Ready (&PyCursor_Type) < 0)
        goto fail;
    
    Py_INCREF (&PyCursor_Type);
    
#if PY_VERSION_HEX < 0x03000000
    mod = Py_InitModule3 ("mouse", _mouse_methods, "");
#else
    mod = PyModule_Create (&_module);
#endif
    if (!mod)
        goto fail;

    PyModule_AddObject (mod, "Cursor", (PyObject *) &PyCursor_Type);
    
    cursor_export_capi (c_api);

    c_api_obj = PyCObject_FromVoidPtr ((void *) c_api, NULL);
    if (c_api_obj)
        PyModule_AddObject (mod, PYGAME_SDLMOUSE_ENTRY, c_api_obj);    

    cursors = PyImport_ImportModule ("cursors");
    if (cursors)
        PyModule_AddObject (mod, "cursors", cursors);

    if (import_pygame2_base () < 0)
        goto fail;
    if (import_pygame2_sdl_base () < 0)
        goto fail;
    if (import_pygame2_sdl_video () < 0)
        goto fail;

    MODINIT_RETURN(mod);
fail:
    Py_XDECREF (mod);
    MODINIT_RETURN (NULL);
}
