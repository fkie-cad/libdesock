"""
This file tests that sendmmsg works correctly.

Target files:
    - libdesock/src/write.c
"""

import ctypes

import desock
import helper

data = bytes(range(65, 115))
cursor = 0
def _get_data(size):
    global cursor
    ret = bytes(data[cursor: cursor + size])
    assert(len(ret) == size)
    cursor += size
    return ret

def test_sendmmsg():
    fd = desock._debug_instant_fd(0)
    
    assert(desock.sendmmsg(fd, None, 0, 0) == 0)
    
    mmsghdrs = (desock.mmsghdr * 2)()
    mmsghdrs[0] = helper.create_mmsghdr(helper.create_msghdr(iov=helper.create_iovec(5, 5, datafunc=_get_data)))
    mmsghdrs[1] = helper.create_mmsghdr(helper.create_msghdr(iov=helper.create_iovec(5, 5, datafunc=_get_data)))
    
    with helper.StdoutPipe() as pipe:
        assert(desock.sendmmsg(fd, mmsghdrs, 2, 0) == 2)
        assert(pipe.read(50) == data)

def test_sendto():
    data = ctypes.create_string_buffer(bytes(range(128)))
    fd = desock._debug_instant_fd(0)
    
    with helper.StdoutPipe() as pipe:
        assert(desock.sendto(fd, data, 128, 0, None, 0) == 128)
        assert(pipe.read(128) == data[:128])
        
def test_sendmsg():
    global cursor
    cursor = 0
    msghdr = helper.create_msghdr(iov=helper.create_iovec(5, 10, datafunc=_get_data))
    fd = desock._debug_instant_fd(0)
    
    with helper.StdoutPipe() as pipe:
        assert(desock.sendmsg(fd, msghdr, 0) == 50)
        assert(pipe.read(50) == data)
        
def test_writev():
    global cursor
    cursor = 0
    iov = helper.create_iovec(5, 10, datafunc=_get_data)
    fd = desock._debug_instant_fd(0)
    
    with helper.StdoutPipe() as pipe:
        assert(desock.writev(fd, iov, 5) == 50)
        assert(pipe.read(50) == data)
        