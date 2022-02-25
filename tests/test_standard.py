"""
This file tests the standard way of creating a server:
    1. socket()
    2. bind()
    3. listen()
    4. accept()
in all possible combinations of socket domains and types.

Target files:
    - libdesock/src/socket.c
    - libdesock/src/bind.c
    - libdesock/src/listen.c
    - libdesock/src/accept.c
"""

import sys
import ctypes
import random
import os

import helper
import desock

def read_from_socket(fd):
    buf = ctypes.create_string_buffer(256)
    desock.read(fd, buf, 256)
    return buf.value

def test_standard():
    data = b"test123"
    with helper.StdinPipe() as pipe:
        for domain in helper.all_domains:
            for type in helper.all_types:
                s, c = helper.get_fake_connection(domain, type)
                pipe.write(data)
                
                assert(read_from_socket(c) == data)
                
                desock.close(c)
                desock.close(s)

successful = 0

def _handle_connection(fd, data):
    global successful
    buf = ctypes.create_string_buffer(len(data))
    n = desock.read(fd, buf, len(data))
    m = desock.write(fd, buf, n)
    desock.close(fd)
    
    assert(n == len(data))
    assert(m == len(data))
    assert(buf.raw == data)
    
    successful += 1

def test_multithreaded():
    with helper.StdinPipe() as pipe, helper.ThreadPool() as pool, helper.NoStdout():
        s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
        desock.bind(s, None, 0)
        desock.listen(s, 0)
        
        for _ in range(10):
            c = desock.accept(s, None, None)
            data = os.urandom(random.randint(20, 50))
            pipe.write(data)
            pool.spawn(_handle_connection, args=[c, data])
    
    assert(successful == 10)
