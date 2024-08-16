##############################################################################
# This file tests that recv, recvmmsg and readv
# work as expected when reading multiple messages from a file
#
# Target files:
#    - libdesock/src/hook.c
# 
#    Kelly Patterson - Cisco Talos
#         Copyright (C) 2024 Cisco Systems Inc
##############################################################################

import ctypes
import desock
import helper

delim = b'=^..^='

def test_recv_trick_delimiter():

    fd = desock._debug_instant_fd(0)
    with helper.StdinFile() as f:

        delim_test = b'Hello, this is a test string=^..^=A=B^C.D.^=Hello, this is a test string'
        answer1=b'Hello, this is a test string'
        answer2=b'A=B^C.D.^=Hello, this is a test string'

        f.write(delim_test)

        buf = ctypes.create_string_buffer(128)
        ret = desock.recv(fd, buf, 128, 0)
        assert(ret == len(answer1))
        assert(buf.value == answer1)

        buf = ctypes.create_string_buffer(128)
        ret = desock.recv(fd, buf, 128, 0)
        assert(ret == len(answer2))
        assert(buf.value == answer2)   

def test_recv_delimiter_on_boundary():
    fd = desock._debug_instant_fd(0)

    with helper.StdinFile() as f:

        delim_test = b'A'*63+delim+b'B'*62+delim+b'C'*62+delim[:4]
        answer1=b'A'*63
        answer2=b'B'*62
        answer3=b'C'*62+delim[:2]
        answer4=delim[2:4]

        f.write(delim_test)

        buf = ctypes.create_string_buffer(64)
        ret = desock.recv(fd, buf, 64, 0)
        assert(ret == len(answer1))
        assert(buf.value == answer1)

        buf = ctypes.create_string_buffer(64)
        ret = desock.recv(fd, buf, 64, 0)
        assert(ret == len(answer2))
        assert(buf.value == answer2)

        buf = ctypes.create_string_buffer(64)
        ret = desock.recv(fd, buf, 64, 0)
        assert(ret == len(answer3))
        assert(buf.value == answer3)

        buf = ctypes.create_string_buffer(64)
        ret = desock.recv(fd, buf, 64, 0)
        assert(ret == len(answer4))
        assert(buf.value == answer4)


def test_recv_multi():
    fd = desock._debug_instant_fd(0)

    with helper.StdinFile() as f:

        delim_test = b'A'*63+delim+b'B'*62+delim+b'C'*85

        f.write(delim_test)
        buf = ctypes.create_string_buffer(128)
        ret = desock.recv(fd, buf, 128, 0)
        assert(ret == 63)
        assert(buf.value == b'A'*63)

        buf = ctypes.create_string_buffer(128)
        ret = desock.recv(fd, buf, 128, 0)
        assert(ret == 62)
        assert(buf.value == b'B'*62)

        buf = ctypes.create_string_buffer(128)
        ret = desock.recv(fd, buf, 128, 0)
        assert(ret == 85)
        assert(buf.value == b'C'*85)


def test_no_delim():
    fd = desock._debug_instant_fd(0)

    with helper.StdinFile() as f:

        data = bytes(range(128))

        f.write(data)
        buf = ctypes.create_string_buffer(128)
        assert(desock.recv(fd, buf, 128, 0) == 128)
        assert(buf.raw == data)

        assert(desock.recv(fd, buf, 128, 0) == 0)

