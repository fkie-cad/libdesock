"""
This file tests the accept() implementation.

Target files:
    - libdesock/src/accept.c
"""

import ctypes

import helper
import desock
        
def test_sockaddr_in():
    for method in [desock.accept, desock.accept4]:
        s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        desock.listen(s, 0)
        addr = desock.sockaddr_in()
        len = ctypes.c_int(ctypes.sizeof(addr))
        c = method(s, addr, len)
        desock.close(c)
        desock.close(s)
        assert(addr.sin_family == desock.AF_INET)
        assert(addr.sin_port == 53764)
        assert(addr.sin_addr == 0x100007f)
    
def test_sockaddr_in6():
    for method in [desock.accept, desock.accept4]:
        s = desock.socket(desock.AF_INET6, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        desock.listen(s, 0)
        addr = desock.sockaddr_in6()
        len = ctypes.c_int(ctypes.sizeof(addr))
        c = method(s, addr, len)
        desock.close(c)
        desock.close(s)
        assert(addr.sin6_family == desock.AF_INET6)
        assert(addr.sin6_port == 53764)
        assert(addr.sin6_flowinfo == 0)
        for i in range(15):
            assert(addr.sin6_addr[i] == 0)
        assert(addr.sin6_addr[15] == 1)
        assert(addr.sin6_scope_id == 0)
    
def test_invalid():
    fd = desock._debug_instant_fd(1)
    for _ in range(fd + 1, desock.FD_TABLE_SIZE):
        desock.dup(0)
    
    try:
        desock.accept(fd, None, None)
    except desock.DesockError as e:
        assert(str(e) == "Success")
    else:
        assert(False)
        
    for i in range(fd, desock.FD_TABLE_SIZE + 1):
        desock.close(i)
