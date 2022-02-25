"""
This file tests the bind() implementation.

Target files:
    - libdesock/src/bind.c
"""

import ctypes

import desock
import helper

def test_invalid():
    s1 = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
    s = desock._debug_real_dup2(s1, desock.FD_TABLE_SIZE)
    addr = desock.sockaddr_in()
    addr.sin_family = desock.AF_INET
    addr.sin_port = 53764
    addr.sin_addr = 0x100007f
    desock.bind(s, addr, ctypes.sizeof(addr))
    sockname = desock.sockaddr_in()
    sockname_len = ctypes.c_int(ctypes.sizeof(sockname))
    desock.getsockname(s, sockname, sockname_len)
    assert(sockname_len.value == ctypes.sizeof(sockname))
    assert(sockname.sin_family == desock.AF_INET)
    assert(sockname.sin_port == 53764)
    assert(sockname.sin_addr == 0x100007f)
    desock.close(s)
    desock.close(s1)
