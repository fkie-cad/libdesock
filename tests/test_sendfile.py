"""
This file tests the sendfile() and sendfile64() functionality.

Target files:
    - libdesock/src/sendfile.c
"""

import os
import ctypes
import tempfile

import helper
import desock

def test_sendfile():
    offset = ctypes.c_long(0)
    data = b"hello motherfucker"
    s, c = helper.get_fake_connection(desock.AF_INET, desock.SOCK_STREAM)
    assert(desock.sendfile(c, -1, offset, len(data)) == len(data))
    desock.close(c)
    desock.close(s)

def test_sendfile64():
    offset = ctypes.c_long(0)
    data = b"hello motherfucker"
    s, c = helper.get_fake_connection(desock.AF_INET, desock.SOCK_STREAM)
    assert(desock.sendfile64(c, -1, offset, len(data)) == len(data))
    desock.close(c)
    desock.close(s)
