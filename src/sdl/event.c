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
#define PYGAME_SDLEVENT_INTERNAL

#include <SDL_syswm.h>
#include "eventmod.h"
#include "pgsdl.h"

static int _set_item (PyObject *dict, char *key, PyObject *value);
static int _get_item (PyObject *dict, char *key, PyObject **value);

static PyObject* _create_dict_from_event (SDL_Event *event);
static int _create_event_from_dict (PyObject *dict, SDL_Event *event);
static PyObject* _event_new (PyTypeObject *type, PyObject *args,
    PyObject *kwds);
static int _event_init (PyObject *event, PyObject *args, PyObject *kwds);
static void _event_dealloc (PyEvent *self);

static PyObject* _event_gettype (PyObject *self, void *closure);
static PyObject* _event_getname (PyObject *self, void *closure);

/**
 */
/*
static PyMethodDef _event_methods[] = {
    { NULL, NULL, 0, NULL }
};
*/

/**
 */
static PyGetSetDef _event_getsets[] = {
    { "type", _event_gettype, NULL, "", NULL },
    { "name", _event_getname, NULL, "", NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

/**
 */
PyTypeObject PyEvent_Type =
{
    TYPE_HEAD(NULL, 0)
    "event.Event",              /* tp_name */
    sizeof (PyEvent),           /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor) _event_dealloc, /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "",
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    _event_getsets,             /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof (PyEvent, dict),   /* tp_dictoffset */
    (initproc) _event_init,     /* tp_init */
    0,                          /* tp_alloc */
    _event_new,                 /* tp_new */
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
_event_dealloc (PyEvent *self)
{
    Py_XDECREF (self->dict);
    ((PyObject*)self)->ob_type->tp_free ((PyObject *) self);
}

static PyObject*
_event_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyEvent *event = (PyEvent *) type->tp_alloc (type, 0);
    if (!event)
        return NULL;
    event->dict = PyDict_New ();
    if (!event->dict)
    {
        Py_DECREF (event);
        return NULL;
    }
    event->type = 0;
    return (PyObject*) event;
}

static int
_event_init (PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *dict = NULL;
    int eventid;
    PyEvent *event = (PyEvent *) self;

    if (!PyArg_ParseTuple (args, "i|O", &eventid, &dict))
        return -1;

    if (dict && !PyDict_Check (dict))
    {
        PyErr_SetString (PyExc_TypeError, "dict must be a dict");
        return -1;
    }

    event->type = eventid;
    
    if (PyDict_Update (event->dict, dict) == -1)
        return -1;

    return 0;
}

static int
_set_item (PyObject *dict, char *key, PyObject *value)
{
    if (value)
    {
        int ret = PyDict_SetItemString (dict, key, value);
        Py_DECREF (value);
        return (ret == 0) ? 1 : 0;
    }
    return 0;
}

static int
_get_item (PyObject *dict, char *key, PyObject **value)
{
    *value = PyDict_GetItemString (dict, key);
    if (!(*value))
    {
        PyErr_Format (PyExc_ValueError, "event dict misses '%s' item", key);
        return 0;
    }
    return 1;
}

static PyObject*
_create_dict_from_event (SDL_Event *event)
{
    PyObject *val = NULL;
    PyObject *dict = PyDict_New ();
    if (!dict)
        return NULL;

    if (event->type >= SDL_USEREVENT && event->type < SDL_NUMEVENTS)
    {
        if (!_set_item (dict, "code", PyInt_FromLong (event->user.code)))
            goto failed;
        /* TODO: user data ptrs. */
        return dict;
    }

    switch (event->type)
    {
    case SDL_ACTIVEEVENT:
        if (!_set_item (dict, "gain", PyInt_FromLong (event->active.gain)))
            goto failed;
        if (!_set_item (dict, "state", PyInt_FromLong (event->active.state)))
            goto failed;
        break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (!_set_item (dict, "state", PyInt_FromLong (event->key.state)))
            goto failed;
        if (!_set_item (dict, "scancode",
                PyInt_FromLong (event->key.keysym.scancode)))
            goto failed;
        if (!_set_item (dict, "key", PyInt_FromLong (event->key.keysym.sym)))
            goto failed;
        if (!_set_item (dict, "sym", PyInt_FromLong (event->key.keysym.sym)))
            goto failed;
        if (!_set_item (dict, "mod", PyInt_FromLong (event->key.keysym.mod)))
            goto failed;
        if (!_set_item (dict, "unicode",
                PyInt_FromLong (event->key.keysym.unicode)))
            goto failed;
        break;

    case SDL_MOUSEMOTION:
        if (!_set_item (dict, "state", PyInt_FromLong (event->motion.state)))
            goto failed;
        if (!_set_item (dict, "x", PyInt_FromLong (event->motion.x)))
            goto failed;
        if (!_set_item (dict, "y", PyInt_FromLong (event->motion.y)))
            goto failed;
        if (!_set_item (dict, "xrel", PyInt_FromLong (event->motion.xrel)))
            goto failed;
        if (!_set_item (dict, "yrel", PyInt_FromLong (event->motion.yrel)))
            goto failed;
        val = Py_BuildValue ("(ii)", event->motion.x, event->motion.y);
        if (!val)
            goto failed;
        if (!_set_item (dict, "pos", val))
            goto failed;
        val = Py_BuildValue ("(ii)", event->motion.xrel, event->motion.yrel);
        if (!val)
            goto failed;
        if (!_set_item (dict, "rel", val))
            goto failed;
        val = PyTuple_New (3);
        if (!val)
            return NULL;
        PyTuple_SET_ITEM (val, 0,
            PyBool_FromLong (event->motion.state & SDL_BUTTON(1)));
        PyTuple_SET_ITEM (val, 0,
            PyBool_FromLong (event->motion.state & SDL_BUTTON(2)));
        PyTuple_SET_ITEM (val, 0,
            PyBool_FromLong (event->motion.state & SDL_BUTTON(3)));
        if (!_set_item (dict, "buttons", val))
            goto failed;
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        if (!_set_item (dict, "button", PyInt_FromLong (event->button.button)))
            goto failed;
        if (!_set_item (dict, "x", PyInt_FromLong (event->button.x)))
            goto failed;
        if (!_set_item (dict, "y", PyInt_FromLong (event->button.y)))
            goto failed;
        val = Py_BuildValue ("(ii)", event->button.x, event->button.y);
        if (!val)
            goto failed;
        if (!_set_item (dict, "pos", val))
            goto failed;
        if (!_set_item (dict, "state", PyInt_FromLong (event->button.state)))
            goto failed;
        break;

    case SDL_JOYAXISMOTION:
        if (!_set_item (dict, "which", PyInt_FromLong (event->jaxis.which)))
            goto failed;
        if (!_set_item (dict, "joy", PyInt_FromLong (event->jaxis.which)))
            goto failed;
        if (!_set_item (dict, "axis", PyInt_FromLong (event->jaxis.axis)))
            goto failed;
        if (!_set_item (dict, "value", PyInt_FromLong (event->jaxis.value)))
            goto failed;
        break;

    case SDL_JOYBALLMOTION:
        if (!_set_item (dict, "which", PyInt_FromLong (event->jball.which)))
            goto failed;
        if (!_set_item (dict, "joy", PyInt_FromLong (event->jball.which)))
            goto failed;
        if (!_set_item (dict, "ball", PyInt_FromLong (event->jball.ball)))
            goto failed;
        if (!_set_item (dict, "xrel", PyInt_FromLong (event->jball.xrel)))
            goto failed;
        if (!_set_item (dict, "yrel", PyInt_FromLong (event->jball.yrel)))
            goto failed;
        val = Py_BuildValue ("(ii)", event->jball.xrel, event->jball.yrel);
        if (!val)
            goto failed;
        if (!_set_item (dict, "rel", val))
            goto failed;
        break;

    case SDL_JOYHATMOTION:
        if (!_set_item (dict, "which", PyInt_FromLong (event->jhat.which)))
            goto failed;
        if (!_set_item (dict, "joy", PyInt_FromLong (event->jhat.which)))
            goto failed;
        if (!_set_item (dict, "hat", PyInt_FromLong (event->jhat.hat)))
            goto failed;
        if (!_set_item (dict, "value", PyInt_FromLong (event->jhat.value)))
            goto failed;
        break;

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        if (!_set_item (dict, "which", PyInt_FromLong (event->jbutton.which)))
            goto failed;
        if (!_set_item (dict, "joy", PyInt_FromLong (event->jbutton.which)))
            goto failed;
        if (!_set_item (dict, "button", PyInt_FromLong (event->jbutton.button)))
            goto failed;
        if (!_set_item (dict, "state", PyInt_FromLong (event->jbutton.state)))
            goto failed;
        break;

    case SDL_VIDEORESIZE:
        if (!_set_item (dict, "w", PyInt_FromLong (event->resize.w)))
            goto failed;
        if (!_set_item (dict, "h", PyInt_FromLong (event->resize.h)))
            goto failed;
        val = Py_BuildValue ("(ii)", event->resize.w, event->resize.h);
        if (!val)
            goto failed;
        if (!_set_item (dict, "size", val))
            goto failed;
        break;

    case SDL_SYSWMEVENT:
#if defined(SDL_VIDEO_DRIVER_WINDIB) || defined(SDL_VIDEO_DRIVER_DDRAW) || defined(SDL_VIDEO_DRIVER_GAPI)
        if (!_set_item (dict, "hwnd",
                PyInt_FromLong ((long) event->syswm.msg->hwnd)))
            goto failed;
        if (!_set_item (dict, "msg", PyInt_FromLong (event->syswm.msg->msg)))
            goto failed;
        if (!_set_item (dict, "wparam",
                PyInt_FromLong (event->syswm.msg->wParam)))
            goto failed;
        if (!_set_item (dict, "lparam",
                PyInt_FromLong (event-> syswm.msg->lParam)))
            goto failed;
#elif defined(SDL_VIDEO_DRIVER_X11)
        if (!_set_item (dict,  "event", PyString_FromStringAndSize
                ((char*) &(event->syswm.msg->event.xevent), sizeof (XEvent))))
            goto failed;
#endif
        break;
    case SDL_QUIT:
        break;
    case SDL_VIDEOEXPOSE:
        break;
    default:
        break;
    }

    return dict;

failed:
    Py_XDECREF (val);
    Py_XDECREF (dict);
    return NULL;
}

static int
_create_event_from_dict (PyObject *dict, SDL_Event *event)
{
    PyObject *val, *tuple;
    
    if (event->type >= SDL_USEREVENT && event->type < SDL_NUMEVENTS)
    {
        val = PyDict_GetItemString (dict, "code");
        event->user.code = PyInt_AsLong (val);
        if (event->user.code == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        return 1;
    }

    switch (event->type)
    {
    case SDL_ACTIVEEVENT:
        if (!_get_item (dict, "gain", &val))
            return 0;
        event->active.gain = PyInt_AsLong (val);
        if (event->active.gain == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "state", &val))
            return 0;
        event->active.state = PyInt_AsLong (val);
        if (event->active.state == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (!_get_item (dict, "state", &val))
            return 0;
        event->key.state = PyInt_AsLong (val);
        if (event->key.state == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "scancode", &val))
            return 0;
        event->key.keysym.scancode = PyInt_AsLong (val);
        if (event->key.keysym.scancode == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "key", &val))
            return 0;
        event->key.keysym.sym = PyInt_AsLong (val);
        if (event->key.keysym.sym == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "mod", &val))
            return 0;
        event->key.keysym.mod = PyInt_AsLong (val);
        if (event->key.keysym.mod == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "unicode", &val))
            return 0;
        event->key.keysym.unicode = PyInt_AsLong (val);
        if (event->key.keysym.unicode == (Uint16)-1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_MOUSEMOTION:
        if (!_get_item (dict, "state", &val))
            return 0;
        event->motion.state = PyInt_AsLong (val);
        if (event->motion.state == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "x", &val))
            return 0;
        event->motion.x = PyInt_AsLong (val);
        if (event->motion.x == (Uint16)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "y", &val))
            return 0;
        event->motion.y = PyInt_AsLong (val);
        if (event->motion.y == (Uint16)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "xrel", &val))
            return 0;
        event->motion.xrel = PyInt_AsLong (val);
        if (event->motion.xrel == -1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "yrel", &val))
            return 0;
        event->motion.yrel = PyInt_AsLong (val);
        if (event->motion.yrel == -1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "buttons", &tuple))
            return 0;
        if (!PyTuple_Check (tuple) || PyTuple_Size (tuple) != 3)
        {
            PyErr_SetString (PyExc_ValueError,
                "buttons value for motion event must be a tuple");
            return 0;
        }
        val = PyTuple_GetItem (tuple, 0);
        if (!val)
            return 0;
        if (val == Py_True)
            event->motion.state |= SDL_BUTTON(1);
        val = PyTuple_GetItem (tuple, 1);
        if (!val)
            return 0;
        if (val == Py_True)
            event->motion.state |= SDL_BUTTON(2);
        val = PyTuple_GetItem (tuple, 2);
        if (!val)
            return 0;
        if (val == Py_True)
            event->motion.state |= SDL_BUTTON(3);
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        if (!_get_item (dict, "button", &val))
            return 0;
        event->button.button = PyInt_AsLong (val);
        if (event->button.button == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "x", &val))
            return 0;
        event->button.x = PyInt_AsLong (val);
        if (event->button.x == (Uint16)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "y", &val))
            return 0;
        event->button.y = PyInt_AsLong (val);
        if (event->button.y == (Uint16)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "state", &val))
            return 0;
        event->button.state = PyInt_AsLong (val);
        if (event->button.state == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_JOYAXISMOTION:
        if (!_get_item (dict, "which", &val))
            return 0;
        event->jaxis.which = PyInt_AsLong (val);
        if (event->jaxis.which == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "axis", &val))
            return 0;
        event->jaxis.axis = PyInt_AsLong (val);
        if (event->jaxis.axis == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "value", &val))
            return 0;
        event->jaxis.value = PyInt_AsLong (val);
        if (event->jaxis.value == -1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_JOYBALLMOTION:
        if (!_get_item (dict, "which", &val))
            return 0;
        event->jball.which = PyInt_AsLong (val);
        if (event->jball.which == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "ball", &val))
            return 0;
        event->jball.ball = PyInt_AsLong (val);
        if (event->jball.ball == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "xrel", &val))
            return 0;
        event->jball.xrel = PyInt_AsLong (val);
        if (event->jball.xrel == -1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "yrel", &val))
            return 0;
        event->jball.yrel = PyInt_AsLong (val);
        if (event->jball.yrel == -1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_JOYHATMOTION:
        if (!_get_item (dict, "which", &val))
            return 0;
        event->jhat.which = PyInt_AsLong (val);
        if (event->jhat.which == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "hat", &val))
            return 0;
        event->jhat.hat = PyInt_AsLong (val);
        if (event->jhat.hat == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "value", &val))
            return 0;
        event->jhat.value = PyInt_AsLong (val);
        if (event->jhat.value == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        if (!_get_item (dict, "which", &val))
            return 0;
        event->jbutton.which = PyInt_AsLong (val);
        if (event->jbutton.which == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "button", &val))
            return 0;
        event->jbutton.button = PyInt_AsLong (val);
        if (event->jbutton.button == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "state", &val))
            return 0;
        event->jbutton.state = PyInt_AsLong (val);
        if (event->jbutton.state == (Uint8)-1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_VIDEORESIZE:
        if (!_get_item (dict, "w", &val))
            return 0;
        event->resize.w = PyInt_AsLong (val);
        if (event->resize.w == -1 && PyErr_Occurred ())
            return 0;
        if (!_get_item (dict, "h", &val))
            return 0;
        event->resize.h = PyInt_AsLong (val);
        if (event->resize.h == -1 && PyErr_Occurred ())
            return 0;
        break;

    case SDL_SYSWMEVENT:
        return 0;
    case SDL_QUIT:
        return 0;
    case SDL_VIDEOEXPOSE:
        return 0;
    }
    return 1;
}


/* Event getters/setters */
static PyObject*
_event_gettype (PyObject *self, void *closure)
{
    return PyInt_FromLong (((PyEvent*)self)->type);
}

static PyObject*
_event_getname (PyObject *self, void *closure)
{
    switch (((PyEvent*)self)->type)
    {
    case SDL_ACTIVEEVENT:
        return PyString_FromString ("ActiveEvent");
    case SDL_KEYDOWN:
        return PyString_FromString ("KeyDown");
    case SDL_KEYUP:
        return PyString_FromString ("KeyUp");
    case SDL_MOUSEMOTION:
        return PyString_FromString ("MouseMotion");
    case SDL_MOUSEBUTTONDOWN:
        return PyString_FromString ("MouseButtonDown");
    case SDL_MOUSEBUTTONUP:
        return PyString_FromString ("MouseButtonUp");
    case SDL_JOYAXISMOTION:
        return PyString_FromString ("JoyAxisMotion");
    case SDL_JOYBALLMOTION:
        return PyString_FromString ("JoyBallMotion");
    case SDL_JOYHATMOTION:
        return PyString_FromString ("JoyHatMotion");
    case SDL_JOYBUTTONUP:
        return PyString_FromString ("JoyButtonUp");
    case SDL_JOYBUTTONDOWN:
        return PyString_FromString ("JoyButtonDown");
    case SDL_QUIT:
        return PyString_FromString ("Quit");
    case SDL_SYSWMEVENT:
        return PyString_FromString ("SysWMEvent");
    case SDL_VIDEORESIZE:
        return PyString_FromString ("VideoResize");
    case SDL_VIDEOEXPOSE:
        return PyString_FromString ("VideoExpose");
    case SDL_NOEVENT:
        return PyString_FromString ("NoEvent");
    }
    if (((PyEvent*)self)->type >= SDL_USEREVENT &&
        ((PyEvent*)self)->type < SDL_NUMEVENTS)
        return PyString_FromString ("UserEvent");
    return PyString_FromString ("Unknown");
}

/* C API */
PyObject*
PyEvent_New (SDL_Event *event)
{
    PyObject *dict;
    PyEvent *ev;

    ev = (PyEvent*) PyEvent_Type.tp_new (&PyEvent_Type, NULL, NULL);
    if (!ev)
        return NULL;

    if (!event)
    {
        ev->type = SDL_NOEVENT;
        ev->dict = PyDict_New ();
        return (PyObject*) ev;
    }

    dict = _create_dict_from_event (event);
    if (!dict)
    {
        Py_DECREF (ev);
        return NULL;
    }

    ev->dict = dict;
    ev->type = event->type;
    
    return (PyObject*) ev;
}

int
PyEvent_SDLEventFromEvent (PyObject *ev, SDL_Event *event)
{
    if (!PyEvent_Check (ev))
    {
        PyErr_SetString (PyExc_TypeError, "event must be an Event");
        return 0;
    }
    if (!((PyEvent*)ev)->dict)
        return 0;

    event->type = ((PyEvent*)ev)->type;
    if (!_create_event_from_dict (((PyEvent*)ev)->dict, event))
        return 0;
    return 1;
}

void
event_export_capi (void **capi)
{
    capi[PYGAME_SDLEVENT_FIRSTSLOT] = &PyEvent_Type;
    capi[PYGAME_SDLEVENT_FIRSTSLOT+1] = PyEvent_New;
    capi[PYGAME_SDLEVENT_FIRSTSLOT+2] = PyEvent_SDLEventFromEvent;
}
