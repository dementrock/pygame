################################################################################
"""

Modification of http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/440554

"""

#################################### IMPORTS ###################################

import os
import subprocess
import errno
import time
import sys
import unittest
import tempfile

if subprocess.mswindows:
    from win32file import ReadFile, WriteFile
    from win32pipe import PeekNamedPipe
    import win32api
    import msvcrt
    
else:
    from signal import SIGINT, SIGTERM, SIGKILL
    import select
    import fcntl

################################### CONSTANTS ##################################

PIPE = subprocess.PIPE

################################################################################

# import signal

# cludge: try to mimic the standard library kill
# signal.SIGKILL = 120

# def kill(pid, sig):
#     cludge: try to mimic the standard library kill
#     assert sig == signal.SIGKILL 
#     handle = win32api.OpenProcess(win32con.PROCESS_TERMINATE, 0, pid)
#     win32api.TerminateProcess(handle, 1) # exit status

# def waitpid(pid):
#     handle = win32api.OpenProcess(win32con.SYNCHRONIZE|win32con .PROCESS_QUERY_INFORMATION , 0, pid)
#     win32event.WaitForSingleObject(handle, win32event.INFINITE)
#     exitCode = win32process.GetExitCodeProcess(handle)

# return pid, exitCode

################################################################################

class Popen(subprocess.Popen):
    def recv(self, maxsize=None):
        return self._recv('stdout', maxsize)
    
    def recv_err(self, maxsize=None):
        return self._recv('stderr', maxsize)

    def send_recv(self, input='', maxsize=None):
        return self.send(input), self.recv(maxsize), self.recv_err(maxsize)
    
    def read_async(self,  wait=.1, e=1, tr=5, stderr=0):
        if tr < 1:
            tr = 1
        x = time.time()+ wait
        y = []
        r = ''
        pr = self.recv
        if stderr:
            pr = self.recv_err
        while time.time() < x or r:
            r = pr()
            if r is None:
                if e:
                    raise Exception("Other end disconnected!")
                else:
                    break
            elif r:
                y.append(r)
            else:
                time.sleep(max((x-time.time())/tr, 0))
        return ''.join(y)
        
    def send_all(self, data):
        while len(data):
            sent = self.send(data)
            if sent is None:
                raise Exception("Other end disconnected!")
            data = buffer(data, sent)
    
    def get_conn_maxsize(self, which, maxsize):
        if maxsize is None:
            maxsize = 1024
        elif maxsize < 1:
            maxsize = 1
        return getattr(self, which), maxsize
    
    def _close(self, which):
        getattr(self, which).close()
        setattr(self, which, None)
    
    if subprocess.mswindows:
        def kill(self):
            # Recipes
            #http://me.in-berlin.de/doc/python/faq/windows.html#how-do-i-emulate-os-kill-in-windows
            #http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/347462
            
            """kill function for Win32"""
            try:
                # This works as well
                # win32api.TerminateProcess(int(self._handle), 0)
                                           # handle,  # exit code

                handle = win32api.OpenProcess(1, 0, self.pid)
                try:
                    win32api.TerminateProcess(handle, 0)
                finally:
                    win32api.CloseHandle(handle)
            except win32api.error:
                return False
            return True

        def send(self, input):
            if not self.stdin:
                return None

            try:
                x = msvcrt.get_osfhandle(self.stdin.fileno())
                (errCode, written) = WriteFile(x, input)
            except ValueError:
                return self._close('stdin')
            except (subprocess.pywintypes.error, Exception), why:
                if why[0] in (109, errno.ESHUTDOWN):
                    return self._close('stdin')
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None
            
            try:
                x = msvcrt.get_osfhandle(conn.fileno())
                (read, nAvail, nMessage) = PeekNamedPipe(x, 0)
                if maxsize < nAvail:
                    nAvail = maxsize
                if nAvail > 0:
                    (errCode, read) = ReadFile(x, nAvail, None)
            except ValueError:
                return self._close(which)
            except (subprocess.pywintypes.error, Exception), why:
                if why[0] in (109, errno.ESHUTDOWN):
                    return self._close(which)
                raise
            
            if self.universal_newlines:
                read = self._translate_newlines(read)
            return read

    else:
        def kill(self):
            # TODO make return val consistent with windows
            
            # waitpid
             
            # and return a tuple containing its pid and exit status
            # indication: a 16-bit number, whose low byte is the
            # signal number that killed the process, and whose high
            # byte is the exit status (if the signal number is
            # zero); the high bit of the low byte is set if a core
            # file was produced. Availability: Macintosh, Unix.

            try:
                for i, sig in enumerate([SIGTERM, SIGKILL] * 2):
                    if i % 2 == 0:  os.kill(self.pid, sig)
                    else:           time.sleep((i+1)/10.0)

                    killed_pid, stat = os.waitpid(self.pid, os.WNOHANG)

                    # print (i, killed_pid, stat)

                    if killed_pid != 0: return True  # ???
            except OSError:
                pass

            return False

        def send(self, input):
            if not self.stdin:
                return None

            if not select.select([], [self.stdin], [], 0)[1]:
                return 0

            try:
                written = os.write(self.stdin.fileno(), input)
            except OSError, why:
                if why[0] == errno.EPIPE: #broken pipe
                    return self._close('stdin')
                raise

            return written

        def _recv(self, which, maxsize):
            conn, maxsize = self.get_conn_maxsize(which, maxsize)
            if conn is None:
                return None
            
            flags = fcntl.fcntl(conn, fcntl.F_GETFL)
            if not conn.closed:
                fcntl.fcntl(conn, fcntl.F_SETFL, flags| os.O_NONBLOCK)
            
            try:
                if not select.select([conn], [], [], 0)[0]:
                    return ''
                
                r = conn.read(maxsize)
                if not r:
                    return self._close(which)
    
                if self.universal_newlines:
                    r = self._translate_newlines(r)
                return r
            finally:
                if not conn.closed:
                    fcntl.fcntl(conn, fcntl.F_SETFL, flags)

################################################################################

def proc_in_time_or_kill(cmd, time_out):
    proc = Popen (
        cmd, bufsize = -1,
        stdin = subprocess.PIPE, stdout = subprocess.PIPE, 
        stderr = subprocess.STDOUT, universal_newlines = 1
    )

    ret_code = None
    response = []

    t = time.time()
    while ret_code is None and ((time.time() -t) < time_out):
        ret_code = proc.poll()
        response += [proc.read_async(wait=0.1, e=0)]

    if ret_code is None:
        proc.kill()
        ret_code = '"Process timed out (time_out = %s secs)"' % time_out

    return ret_code, ''.join(response)

################################################################################

class AsyncTest(unittest.TestCase):
    def test_proc_in_time_or_kill(self):
        ret_code, response = proc_in_time_or_kill(
            ['python.exe', '-c', 'while 1: pass'], time_out = 1
        )
        self.assert_( ret_code.startswith('"Process timed out') )

################################################################################

def _example():
    if sys.platform == 'win32':
        shell, commands, tail = ('cmd', ('echo "hello"', 'echo "HELLO WORLD"'), '\r\n')
    else:
        shell, commands, tail = ('sh', ('ls', 'echo HELLO WORLD'), '\n')
    
    a = Popen(shell, stdin=PIPE, stdout=PIPE)
    print a.read_async(),
    for cmd in commands:
        a.send_all(cmd + tail)
        print a.read_async(),
    a.send_all('exit' + tail)
    print a.read_async(e=0)
    a.wait()

################################################################################
    
if __name__ == '__main__':
    if 1:
        unittest.main()
    else:
        _example()