##############################################################################
# Added StdinFile class   
#    Kelly Patterson - Cisco Talos
#         Copyright (C) 2024 Cisco Systems Inc
##############################################################################
import os
import ctypes
import socket
import threading
import random
import tempfile

import desock

all_domains = [
    desock.AF_INET,
    desock.AF_INET6
]
all_types = [
    desock.SOCK_STREAM,
    desock.SOCK_DGRAM
]

class StdinPipe:
    def __init__(self):
        self._r_fd = None
        self._w_fd = None
        self._stdin_backup = None
    
    def __enter__(self):
        self._r_fd, self._w_fd = os.pipe()
        self._stdin_backup = os.dup(0)
        os.dup2(self._r_fd, 0)
        return self
        
    def write(self, data):
        return os.write(self._w_fd, bytes(data))
        
    def close(self):
        if self._w_fd is not None:
            os.close(self._w_fd)
            self._w_fd = None
        
    def __exit__(self, *args):
        self.close()
        os.dup2(self._stdin_backup, 0)
        os.close(self._stdin_backup)
        os.close(self._r_fd)

class StdinFile:
    def __init__(self):
        self._stdin_backup = None
        self.tmpFile = None
        self.tmpFd = None

    def __enter__(self):
        self.tmpFile = tempfile.NamedTemporaryFile()
        self._stdin_backup = os.dup(0)
        self.tmpFd = os.open(self.tmpFile.name, os.O_RDWR)
        os.dup2(self.tmpFd, 0)
        return self

    def write(self, data):
        os.write(0, data)
        os.lseek(0, 0, os.SEEK_SET)

    def __exit__(self, *args):
        os.dup2(self._stdin_backup, 0)
        os.close(self._stdin_backup)
        os.close(self.tmpFd)
        self.tmpFile.close()
        
class StdoutPipe:
    def __init__(self):
        self._r_fd = None
        self._w_fd = None
        self._stdout_backup = None
        
    def __enter__(self):
        self._r_fd, self._w_fd = os.pipe()
        self._stdout_backup = os.dup(1)
        os.dup2(self._w_fd, 1)
        return self
        
    def read(self, n):
        return os.read(self._r_fd, n)
        
    def __exit__(self, *args):
        os.close(self._r_fd)
        os.close(self._w_fd)
        os.dup2(self._stdout_backup, 1)
        os.close(self._stdout_backup)
        
def _init_data(size):
    return bytes([0xff] * size)
    
def create_iovec(number, size, datafunc=_init_data):
    iovs = (desock.iovec * number)()
    for i in range(number):
        iovs[i].iov_len = size
        
        if size > 0:
            iovs[i].iov_base = ctypes.create_string_buffer(datafunc(size))
        else:
            iovs[i].iov_base = None
    return iovs
        
def create_msghdr(iov=None, namelen=0, controllen=0, flags=0, datafunc=_init_data):
    msghdr = desock.msghdr()
    msghdr.msg_namelen = namelen
    msghdr.msg_controllen = controllen
    msghdr.msg_flags = flags
    
    if namelen > 0:
        msghdr.msg_name = ctypes.create_string_buffer(datafunc(namelen))
    else:
        msghdr.msg_name = None
    
    if controllen > 0:
        msghdr.msg_control = ctypes.create_string_buffer(datafunc(controllen))
    else:
        msghdr.msg_control = None
    
    msghdr.msg_iov = iov
    
    if iov is not None:
        msghdr.msg_iovlen = len(iov)
    else:
        msghdr.msg_iovlen = 0
    
    return msghdr
    
def create_mmsghdr(msghdr):
    mmsghdr = desock.mmsghdr()
    mmsghdr.msg_hdr = msghdr
    mmsghdr.msg_len = 0
    return mmsghdr
    
def _spawn_real_server(host="127.0.0.1", port=0):
    address = desock.sockaddr_in(desock.AF_INET, socket.htons(port), int.from_bytes(socket.inet_aton(host), "little"))
    s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
    desock._debug_real_bind(s, address, ctypes.sizeof(address))
    desock.listen(s, 10)
    return s
    
def interact_with_real_server(callback, data):
    result = None
    port = random.choice(range(1025, 65535))
    
    def handle_client(s):
        nonlocal callback, data, result
        c = desock.accept(s, None, None)
        try:
            callback(c)
        except BaseException as b:
            result = b
        desock.close(c)
        
    s = _spawn_real_server(port=port)
    t = threading.Thread(target=handle_client, args=(s,))
    t.start()
    
    c = socket.create_connection(("127.0.0.1", port), timeout=3)
    c.send(data)
    c.close()
    
    t.join()
    desock.close(s)
    
    if result is not None:
        raise result

def get_fake_connection(domain, type):
    s = desock.socket(domain, type, 0)
    desock.bind(s, None, 0)
    desock.listen(s, 0)
    c = desock.accept(s, None, None)
    return s, c
    
class ThreadPool:
    def __init__(self):
        self._pool = []
        
    def spawn(self, method, args=[], kwargs={}):
        t = threading.Thread(target=method, args=tuple(args), kwargs=kwargs)
        t.start()
        self._pool.append(t)
        
    def __enter__(self):
        return self
        
    def __exit__(self, *args):
        for t in self._pool:
            t.join() 
            
class NoStdout:
    def __init__(self):
        self._dev_null = None
        self._stdout_backup = None
        
    def __enter__(self):
        self._stdout_backup = os.dup(1)
        self._dev_null = os.open("/dev/null", os.O_WRONLY)
        os.dup2(self._dev_null, 1)
        return self
        
    def __exit__(self, *args):
        os.close(self._dev_null)
        os.dup2(self._stdout_backup, 1)
        os.close(self._stdout_backup)
        
        
