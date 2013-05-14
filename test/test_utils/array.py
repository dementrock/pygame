"""Module pygame.tests.test_utils.array

Export the BufferExporter and BufferImporter classes.

Class BufferExporter has configurable shape and strides. BufferExporter objects
provide a convient target for unit tests on Pygame objects and functions that
import a new buffer interface.

Class BufferImporter imports a buffer interface with the given PyBUF_* flags.
It returns NULL Py_buffer fields as None. The shape, strides, and suboffsets
arrays are returned as tuples of ints. All Py_buffer field properties are
read-only. This class is useful in comparing exported buffer interfaces
with the actual request. The simular Python builtin memoryview currently
does not support configurable PyBUF_* flags.

This module contains its own unit tests. When Pygame is installed, these tests
can be run with the following command line statement:

python -m pygame.tests.test_utils.array

"""
import pygame
if not pygame.HAVE_NEWBUF:
    emsg = "This Pygame build does not support the new buffer protocol"
    raise ImportError(emsg)
import pygame.newbuffer
from pygame.newbuffer import (PyBUF_SIMPLE, PyBUF_FORMAT, PyBUF_ND,
                              PyBUF_WRITABLE, PyBUF_STRIDES, PyBUF_C_CONTIGUOUS,
                              PyBUF_F_CONTIGUOUS, PyBUF_ANY_CONTIGUOUS,
                              PyBUF_INDIRECT, PyBUF_STRIDED, PyBUF_STRIDED_RO,
                              PyBUF_RECORDS, PyBUF_RECORDS_RO, PyBUF_FULL,
                              PyBUF_FULL_RO, PyBUF_CONTIG, PyBUF_CONTIG_RO)

import unittest
import sys
import ctypes
import operator

__all__ = ["BufferExporter", "BufferImporter"]

def _prop_get(fn):
    return property(fn)

class BufferExporter(pygame.newbuffer.BufferMixin):
    """An object that exports a multi-dimension new buffer interface

       The only array operation this type supports is to export a buffer.
    """
    prefixes = {'@': '', '=': '=', '<': '=', '>': '=',
                '!': '=', '1': '1', '2': '2', '3': '3',
                '4': '4', '5': '5', '6': '6', '7': '7',
                '8': '8', '9': '9', 'c': 'c', 'b': 'b',
                'B': 'B', 'h': 'h', 'H': 'H', 'i': 'i',
                'I': 'I', 'l': 'l', 'L': 'L', 'q': 'q',
                'Q': 'Q', 'f': 'f', 'd': 'd', 'P': 'P',
                'x': 'x'}
    types = {'c': ctypes.c_char, 'b': ctypes.c_byte, 'B': ctypes.c_ubyte,
             '=c': ctypes.c_int8, '=b': ctypes.c_int8, '=B': ctypes.c_uint8,
             '?': ctypes.c_bool, '=?': ctypes.c_int8,
             'h': ctypes.c_short, 'H': ctypes.c_ushort,
             '=h': ctypes.c_int16, '=H': ctypes.c_uint16,
             'i': ctypes.c_int, 'I': ctypes.c_uint,
             '=i': ctypes.c_int32, '=I': ctypes.c_uint32,
             'l': ctypes.c_long, 'L': ctypes.c_ulong,
             '=l': ctypes.c_int32, '=L': ctypes.c_uint32,
             'q': ctypes.c_longlong, 'Q': ctypes.c_ulonglong,
             '=q': ctypes.c_int64, '=Q': ctypes.c_uint64,
             'f': ctypes.c_float, 'd': ctypes.c_double,
             'P': ctypes.c_void_p,
             'x': ctypes.c_ubyte * 1,
             '1x': ctypes.c_ubyte * 1,
             '2x': ctypes.c_ubyte * 2,
             '3x': ctypes.c_ubyte * 3,
             '4x': ctypes.c_ubyte * 4,
             '5x': ctypes.c_ubyte * 5,
             '6x': ctypes.c_ubyte * 6,
             '7x': ctypes.c_ubyte * 7,
             '8x': ctypes.c_ubyte * 8,
             '9x': ctypes.c_ubyte * 9}

    def __init__(self, shape, format=None, strides=None, readonly=None):
        if format is None:
            format = 'B'
        if readonly is None:
            readonly = False
        try:
            prefix = self.prefixes[format[0]]
        except LookupError:
            prefix = ' '   # Will fail later
        if len(format) == 2:
            typecode = format[1]
        elif len(format) == 1:
            typecode = ''
        else:
            typecode = ' '   # Will fail later
        try:
            c_itemtype = self.types[prefix + typecode]
        except KeyError:
            raise ValueError("Unknown item format '" + format + "'")
        self.readonly = bool(readonly)
        self.format = format
        self._format = ctypes.create_string_buffer(format.encode('latin_1'))
        self.ndim = len(shape)
        self.itemsize = ctypes.sizeof(c_itemtype)
        self.len = reduce(operator.mul, shape, 1) * self.itemsize
        self.shape = tuple(shape)
        self._shape = (ctypes.c_ssize_t * self.ndim)(*self.shape)
        if strides is None:
            self._strides = (ctypes.c_ssize_t * self.ndim)()
            self._strides[self.ndim - 1] = self.itemsize
            for i in range(self.ndim - 1, 0, -1):
                self._strides[i - 1] = self.shape[i] * self._strides[i]
            self.strides = tuple(self._strides)
        elif len(strides) == self.ndim:
            self.strides = tuple(strides)
            self._strides = (ctypes.c_ssize_t * self.ndim)(*self.strides)
        else:
            raise ValueError("Mismatch in length of strides and shape")
        buflen =  max(self.shape[i] * self.strides[i] for i in range(self.ndim))
        self.buflen = buflen
        self._buf = (ctypes.c_ubyte * buflen)()
        self.buf = ctypes.addressof(self._buf)

    def buffer_info(self):
        return (addressof(self.buffer), self.shape[0])

    def tobytes(self):
        return cast(self.buffer, POINTER(c_char))[0:self._len]

    def __len__(self):
        return self.shape[0]

    def _get_buffer(self, view, flags):
        from ctypes import addressof
        if (flags & PyBUF_WRITABLE) == PyBUF_WRITABLE and self.readonly:
            raise BufferError("buffer is read-only")
        if ((flags & PyBUF_C_CONTIGUOUS) == PyBUF_C_CONTIGUOUS and
            not self.is_contiguous('C')):
            raise BufferError("data is not C contiguous")
        if ((flags & PyBUF_F_CONTIGUOUS) == PyBUF_F_CONTIGUOUS and
            not self.is_contiguous('F')):
            raise BufferError("data is not F contiguous")
        if ((flags & PyBUF_ANY_CONTIGUOUS) == PyBUF_ANY_CONTIGUOUS and
            not self.is_contiguous('A')):
            raise BufferError("data is not contiguous")
        view.buf = self.buf
        view.readonly = self.readonly
        view.len = self.len
        if flags == PyBUF_SIMPLE:
            view.itemsize = 1
            view.ndim = 0
        else:
            view.itemsize = self.itemsize
        if (flags & PyBUF_FORMAT) == PyBUF_FORMAT:
            view.format = addressof(self._format)
        else:
            view.format = None
        if (flags & PyBUF_ND) == PyBUF_ND:
            view.ndim = self.ndim
            view.shape = addressof(self._shape)
        elif self.ndim == 1:
            view.shape = None
        else:
            raise BufferError(
                "shape required for {} dimensional data".format(self.ndim))
        if (flags & PyBUF_STRIDES) == PyBUF_STRIDES:
            view.strides = ctypes.addressof(self._strides)
        elif self.is_contiguous('C'):
            view.strides = None
        else:
            raise BufferError("strides required for none C contiguous data")
        view.suboffsets = None
        view.internal = None
        view.obj = self

    def is_contiguous(self, fortran):
        if fortran in "CA":
            if self.strides[-1] == self.itemsize:
                for i in range(self.ndim - 1, 0, -1):
                    if self.strides[i - 1] != self.shape[i] * self.strides[i]:
                        break
                else:
                    return True
        if fortran in "FA":
            if self.strides[0] == self.itemsize:
                for i in range(0, self.ndim - 1):
                    if self.strides[i + 1] != self.shape[i] * self.strides[i]:
                        break
                else:
                    return True
        return False


class BufferImporter(object):
    """An object that imports a new buffer interface

       The fields of the Py_buffer C struct are exposed by identically
       named BufferImporter read-only properties.
    """
    def __init__(self, obj, flags):
        self._view = pygame.newbuffer.Py_buffer()
        self._view.get_buffer(obj, flags)
    @property
    def obj(self):
        """return object or None for NULL field"""
        return self._view.obj
    @property
    def buf(self):
        """return int or None for NULL field"""
        return self._view.buf
    @property
    def len(self):
        """return int"""
        return self._view.len
    @property
    def readonly(self):
        """return bool"""
        return self._view.readonly
    @property
    def format(self):
        """return bytes or None for NULL field"""
        return ctypes.cast(self._view.format, ctypes.c_char_p).value
    @property
    def itemsize(self):
        """return int"""
        return self._view.itemsize
    @property
    def ndim(self):
        """return int"""
        return self._view.ndim
    @property
    def shape(self):
        """return int tuple or None for NULL field"""
        return self._to_ssize_tuple(self._view.shape)
    @property
    def strides(self):
        """return int tuple or None for NULL field"""
        return self._to_ssize_tuple(self._view.strides)
    @property
    def suboffsets(self):
        """return int tuple or None for NULL field"""
        return self._to_ssize_tuple(self._view.suboffsets)
    @property
    def internal(self):
        """return int or None for NULL field"""
        return self._view.internal

    def _to_ssize_tuple(self, addr):
        from ctypes import cast, POINTER, c_ssize_t

        if addr is None:
            return None
        return tuple(cast(addr, POINTER(c_ssize_t))[0:self._view.ndim])


class BufferExporterTest(unittest.TestCase):
    """Class BufferExporter unit tests"""
    def test_formats(self):
        char_sz = ctypes.sizeof(ctypes.c_char)
        short_sz = ctypes.sizeof(ctypes.c_short)
        int_sz = ctypes.sizeof(ctypes.c_int)
        long_sz = ctypes.sizeof(ctypes.c_long)
        longlong_sz = ctypes.sizeof(ctypes.c_longlong)
        float_sz = ctypes.sizeof(ctypes.c_float)
        double_sz = ctypes.sizeof(ctypes.c_double)
        voidp_sz = ctypes.sizeof(ctypes.c_void_p)

        self.check_args(0, (1,), 'B', (1,), 1, 1, 1)
        self.check_args(1, (1,), 'b', (1,), 1, 1, 1)
        self.check_args(1, (1,), 'B', (1,), 1, 1, 1)
        self.check_args(1, (1,), 'c', (char_sz,), char_sz, char_sz, char_sz)
        self.check_args(1, (1,), 'h', (short_sz,), short_sz, short_sz, short_sz)
        self.check_args(1, (1,), 'H', (short_sz,), short_sz, short_sz, short_sz)
        self.check_args(1, (1,), 'i', (int_sz,), int_sz, int_sz, int_sz)
        self.check_args(1, (1,), 'I', (int_sz,), int_sz, int_sz, int_sz)
        self.check_args(1, (1,), 'l', (long_sz,), long_sz, long_sz, long_sz)
        self.check_args(1, (1,), 'L', (long_sz,), long_sz, long_sz, long_sz)
        self.check_args(1, (1,), 'q', (longlong_sz,),
                        longlong_sz, longlong_sz, longlong_sz)
        self.check_args(1, (1,), 'Q', (longlong_sz,),
                        longlong_sz, longlong_sz, longlong_sz)
        self.check_args(1, (1,), 'f', (float_sz,), float_sz, float_sz, float_sz)
        self.check_args(1, (1,), 'd', (double_sz,),
                        double_sz, double_sz, double_sz)
        self.check_args(1, (1,), 'x', (1,), 1, 1, 1)
        self.check_args(1, (1,), 'P', (voidp_sz,), voidp_sz, voidp_sz, voidp_sz)
        self.check_args(1, (1,), '@b', (1,), 1, 1, 1)
        self.check_args(1, (1,), '@B', (1,), 1, 1, 1)
        self.check_args(1, (1,), '@c', (char_sz,), char_sz, char_sz, char_sz)
        self.check_args(1, (1,), '@h', (short_sz,),
                        short_sz, short_sz, short_sz)
        self.check_args(1, (1,), '@H', (short_sz,),
                        short_sz, short_sz, short_sz)
        self.check_args(1, (1,), '@i', (int_sz,), int_sz, int_sz, int_sz)
        self.check_args(1, (1,), '@I', (int_sz,), int_sz, int_sz, int_sz)
        self.check_args(1, (1,), '@l', (long_sz,), long_sz, long_sz, long_sz)
        self.check_args(1, (1,), '@L', (long_sz,), long_sz, long_sz, long_sz)
        self.check_args(1, (1,), '@q',
             (longlong_sz,), longlong_sz, longlong_sz, longlong_sz)
        self.check_args(1, (1,), '@Q', (longlong_sz,),
                        longlong_sz, longlong_sz, longlong_sz)
        self.check_args(1, (1,), '@f', (float_sz,),
                        float_sz, float_sz, float_sz)
        self.check_args(1, (1,), '@d', (double_sz,),
                        double_sz, double_sz, double_sz)
        self.check_args(1, (1,), '=b', (1,), 1, 1, 1)
        self.check_args(1, (1,), '=B', (1,), 1, 1, 1)
        self.check_args(1, (1,), '=c', (1,), 1, 1, 1)
        self.check_args(1, (1,), '=h', (2,), 2, 2, 2)
        self.check_args(1, (1,), '=H', (2,), 2, 2, 2)
        self.check_args(1, (1,), '=i', (4,), 4, 4, 4)
        self.check_args(1, (1,), '=I', (4,), 4, 4, 4)
        self.check_args(1, (1,), '=l', (4,), 4, 4, 4)
        self.check_args(1, (1,), '=L', (4,), 4, 4, 4)
        self.check_args(1, (1,), '=q', (8,), 8, 8, 8)
        self.check_args(1, (1,), '=Q', (8,), 8, 8, 8)
        self.check_args(1, (1,), '<h', (2,), 2, 2, 2)
        self.check_args(1, (1,), '>h', (2,), 2, 2, 2)
        self.check_args(1, (1,), '!h', (2,), 2, 2, 2)
        self.check_args(1, (1,), '<q', (8,), 8, 8, 8)
        self.check_args(1, (1,), '>q', (8,), 8, 8, 8)
        self.check_args(1, (1,), '!q', (8,), 8, 8, 8)
        self.check_args(1, (1,), '1x', (1,), 1, 1, 1)
        self.check_args(1, (1,), '2x', (2,), 2, 2, 2)
        self.check_args(1, (1,), '3x', (3,), 3, 3, 3)
        self.check_args(1, (1,), '4x', (4,), 4, 4, 4)
        self.check_args(1, (1,), '5x', (5,), 5, 5, 5)
        self.check_args(1, (1,), '6x', (6,), 6, 6, 6)
        self.check_args(1, (1,), '7x', (7,), 7, 7, 7)
        self.check_args(1, (1,), '8x', (8,), 8, 8, 8)
        self.check_args(1, (1,), '9x', (9,), 9, 9, 9)
        self.assertRaises(ValueError, BufferExporter, (2, 1), '')
        self.assertRaises(ValueError, BufferExporter, (2, 1), 'W')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '^Q')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '=W')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '=f')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '=d')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '<f')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '<d')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '>f')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '>d')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '!f')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '!d')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '0x')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '1h')
        self.assertRaises(ValueError, BufferExporter, (2, 1), '11x')
        self.assertRaises(ValueError, BufferExporter, (2, 1), 'BB')

    def test_strides(self):
        self.check_args(1, (10,), '=h', (2,), 20, 20, 2)
        self.check_args(1, (5, 3), '=h', (6, 2), 30, 30, 2)
        self.check_args(1, (7, 3, 5), '=h', (30, 10, 2), 210, 210, 2)
        self.check_args(1, (13, 5, 11, 3), '=h', (330, 66, 6, 2), 4290, 4290, 2)
        self.check_args(3, (7, 3, 5), '=h', (2, 14, 42), 210, 210, 2)
        self.check_args(3, (7, 3, 5), '=h', (2, 16, 48), 210, 240, 2)
        self.check_args(3, (13, 5, 11, 3), '=h', (440, 88, 8, 2), 4290, 5720, 2)
        self.check_args(3, (7, 5), '3x', (15, 3), 105, 105, 3)
        self.check_args(3, (7, 5), '3x', (3, 21), 105, 105, 3)
        self.check_args(3, (7, 5), '3x', (3, 24), 105, 120, 3)

    def test_readonly(self):
        a = BufferExporter((2,), 'h', readonly=True)
        self.assertTrue(a.readonly)
        b = BufferImporter(a, PyBUF_STRIDED_RO)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_STRIDED)
        b = BufferImporter(a, PyBUF_STRIDED_RO)

    def test_is_contiguous(self):
        a = BufferExporter((10,), '=h')
        self.assertTrue(a.is_contiguous('C'))
        self.assertTrue(a.is_contiguous('F'))
        self.assertTrue(a.is_contiguous('A'))
        a = BufferExporter((10, 4), '=h')
        self.assertTrue(a.is_contiguous('C'))
        self.assertTrue(a.is_contiguous('A'))
        self.assertFalse(a.is_contiguous('F'))
        a = BufferExporter((13, 5, 11, 3), '=h', (330, 66, 6, 2))
        self.assertTrue(a.is_contiguous('C'))
        self.assertTrue(a.is_contiguous('A'))
        self.assertFalse(a.is_contiguous('F'))
        a = BufferExporter((10, 4), '=h', (2, 20))
        self.assertTrue(a.is_contiguous('F'))
        self.assertTrue(a.is_contiguous('A'))
        self.assertFalse(a.is_contiguous('C'))
        a = BufferExporter((13, 5, 11, 3), '=h', (2, 26, 130, 1430))
        self.assertTrue(a.is_contiguous('F'))
        self.assertTrue(a.is_contiguous('A'))
        self.assertFalse(a.is_contiguous('C'))
        a = BufferExporter((2, 11, 6, 4), '=h', (576, 48, 8, 2))
        self.assertFalse(a.is_contiguous('A'))
        a = BufferExporter((2, 11, 6, 4), '=h', (2, 4, 48, 288))
        self.assertFalse(a.is_contiguous('A'))
        a = BufferExporter((3, 2, 2), '=h', (16, 8, 4))
        self.assertFalse(a.is_contiguous('A'))
        a = BufferExporter((3, 2, 2), '=h', (4, 12, 24))
        self.assertFalse(a.is_contiguous('A'))

    def test_PyBUF_flags(self):
        a = BufferExporter((10,), 'B')
        b = BufferImporter(a, PyBUF_SIMPLE)
        self.assertTrue(b.obj is a)
        self.assertTrue(b.format is None)
        self.assertEqual(b.len, a.len)
        self.assertEqual(b.itemsize, 1)
        self.assertTrue(b.shape is None)
        self.assertTrue(b.strides is None)
        self.assertTrue(b.suboffsets is None)
        self.assertTrue(b.internal is None)
        self.assertFalse(b.readonly)
        b = BufferImporter(a, PyBUF_ND)
        self.assertTrue(b.obj is a)
        self.assertTrue(b.format is None)
        self.assertEqual(b.len, a.len)
        self.assertEqual(b.itemsize, 1)
        self.assertEqual(b.shape, (10,))
        self.assertTrue(b.strides is None)
        self.assertTrue(b.suboffsets is None)
        self.assertTrue(b.internal is None)
        self.assertFalse(b.readonly)
        a = BufferExporter((5, 10), '=h', (24, 2))
        b = BufferImporter(a, PyBUF_STRIDES)
        self.assertTrue(b.obj is a)
        self.assertTrue(b.format is None)
        self.assertEqual(b.len, 100)
        self.assertEqual(b.itemsize, 2)
        self.assertEqual(b.shape, (5, 10))
        self.assertEqual(b.strides, (24, 2))
        self.assertTrue(b.suboffsets is None)
        self.assertTrue(b.internal is None)
        self.assertFalse(b.readonly)
        b = BufferImporter(a, PyBUF_FULL)
        self.assertTrue(b.obj is a)
        self.assertEqual(b.format, '=h')
        self.assertEqual(b.len, 100)
        self.assertEqual(b.itemsize, 2)
        self.assertEqual(b.shape, (5, 10))
        self.assertEqual(b.strides, (24, 2))
        self.assertTrue(b.suboffsets is None)
        self.assertTrue(b.internal is None)
        self.assertFalse(b.readonly)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_SIMPLE)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_ND)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_C_CONTIGUOUS)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_F_CONTIGUOUS)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_ANY_CONTIGUOUS)
        self.assertRaises(BufferError, BufferImporter, a, PyBUF_CONTIG)

    def test_attributes(self):
        a = BufferExporter((13, 5, 11, 3), '=h', (440, 88, 8, 2))
        self.assertEqual(a.ndim, 4)
        self.assertEqual(a.itemsize, 2)
        self.assertFalse(a.readonly)
        self.assertEqual(a.shape, (13, 5, 11, 3))
        self.assertEqual(a.format, '=h')
        self.assertEqual(a.strides, (440, 88, 8, 2))
        self.assertEqual(a.len, 4290)
        self.assertEqual(a.buflen, 5720)
        self.assertEqual(a.buf, ctypes.addressof(a._buf))
        a = BufferExporter((8,))
        self.assertEqual(a.ndim, 1)
        self.assertEqual(a.itemsize, 1)
        self.assertFalse(a.readonly)
        self.assertEqual(a.shape, (8,))
        self.assertEqual(a.format, 'B')
        self.assertTrue(isinstance(a.strides, tuple))
        self.assertEqual(a.strides, (1,))
        self.assertEqual(a.len, 8)
        self.assertEqual(a.buflen, 8)
        a = BufferExporter([13, 5, 11, 3], '=h', [440, 88, 8, 2])
        self.assertTrue(isinstance(a.shape, tuple))
        self.assertTrue(isinstance(a.strides, tuple))
        self.assertEqual(a.shape, (13, 5, 11, 3))
        self.assertEqual(a.strides, (440, 88, 8, 2))

    def check_args(self, call_flags,
                   shape, format, strides, length, bufsize, itemsize):
        format_arg = format if call_flags & 1 else None
        strides_arg = strides if call_flags & 2 else None
        a = BufferExporter(shape, format_arg, strides_arg)
        self.assertEqual(a.buflen, bufsize)
        m = BufferImporter(a, PyBUF_RECORDS_RO)
        self.assertEqual(m.buf, a.buf)
        self.assertEqual(m.len, length)
        self.assertEqual(m.format, format)
        self.assertEqual(m.itemsize, itemsize)
        self.assertEqual(m.shape, shape)
        self.assertEqual(m.strides, strides)
        
        
if __name__ == '__main__':
    unittest.main()
