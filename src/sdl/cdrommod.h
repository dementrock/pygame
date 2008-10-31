/*
  pygame - Python Game Library
  Copyright (C) 2008 Marcus von Appen

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
#ifndef _PYGAME_SDLCDROM_H_
#define _PYGAME_SDLCDROM_H_

#include <Python.h>
#include <SDL.h>

#define PYGAME_SDLCDROM_INTERNAL
#define PYGAME_SDLCDTRACK_INTERNAL

extern PyTypeObject PyCD_Type;
extern PyTypeObject PyCDTrack_Type;

#define PyCD_Check(x) (PyObject_TypeCheck (x, &PyCD_Type))
#define PyCDTrack_Check(x) (PyObject_TypeCheck (x, &PyCDTrack_Type))
PyObject* PyCD_New (int index);
PyObject* PyCDTrack_New (SDL_CDtrack track);

void cdrom_export_capi (void **capi);
void cdtrack_export_capi (void **capi);

void cdrommod_add_drive (int _index, SDL_CD *cdrom);
void cdrommod_remove_drive (int _index);
SDL_CD* cdrommod_get_drive (int _index);

#endif /* _PYGAME_SDLCDROM_H_ */
