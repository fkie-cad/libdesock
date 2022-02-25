"""
This file tests all functions that retrieve metadata about
a socket or connection.

Target files:
    - libdesock/src/getpeername.c
    - libdesock/src/getsockname.c
"""

import ctypes

import helper
import desock

def test_getpeername():
    for domain in helper.all_domains:
        sockaddr = desock.sockaddr()
        addr_len = ctypes.c_long(ctypes.sizeof(sockaddr))
        s = desock.socket(domain, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        desock.listen(s, 0)
        c = desock.accept(s, None, None)
        r = desock.getpeername(c, sockaddr, addr_len)
        assert(r == 0)
        assert(sockaddr.sa_family == domain)
        assert(addr_len.value == ctypes.sizeof(sockaddr))
        desock.close(s)
        desock.close(c)
        
def test_getsockname():
    for domain in helper.all_domains:
        sockaddr = desock.sockaddr()
        addr_len = ctypes.c_long(ctypes.sizeof(sockaddr))
        s = desock.socket(domain, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        assert(desock.getsockname(s, sockaddr, addr_len) == 0)
        assert(sockaddr.sa_family == domain)
        assert(addr_len.value == ctypes.sizeof(sockaddr))
        desock.close(s)
