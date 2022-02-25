"""
This file tests that recvmsg, recvmmsg and readv
work as expected without MSG_PEEK set.

Target files:
    - libdesock/src/read.c
"""

import os
import random
import ctypes
import tempfile

import desock
import helper

def test_read_syscall():
    fd, _ = tempfile.mkstemp()
    data = (ctypes.c_char * 5)()
    len = ctypes.sizeof(data)
    for i in range(len):
        data[i] = i
    assert(desock.write(fd, data, len) == len)
    os.lseek(fd, 0, 0)
    data = (ctypes.c_char * 5)()
    len = ctypes.sizeof(data)
    assert(desock.read(fd, data, len) == len)
    for i in range(len):
        assert(data[i][0] == i)
    desock.close(fd)

def test_recvfrom():
    # AF_INET
    with helper.StdinPipe() as pipe:
        pipe.write(b"12345")
        pipe.close()
        data = (ctypes.c_char * 128)()
        sockaddr = desock.sockaddr_in()
        sockaddr_len = ctypes.c_int(ctypes.sizeof(sockaddr))
        s = desock.socket(desock.AF_INET, desock.SOCK_STREAM, 0)
        desock.connect(s, None, 0)
        assert(desock.recvfrom(s, data, 128, 0, sockaddr, sockaddr_len) == 5)
        desock.close(s)
        assert(data.value == b"12345")
        assert(sockaddr.sin_family == desock.AF_INET)
        assert(sockaddr.sin_port == 53764)
        assert(sockaddr.sin_addr == 0x100007f)
        
    # AF_INET6
    with helper.StdinPipe() as pipe:
        pipe.write(b"12345")
        pipe.close()
        data = (ctypes.c_char * 128)()
        sockaddr = desock.sockaddr_in6()
        sockaddr_len = ctypes.c_int(ctypes.sizeof(sockaddr))
        s = desock.socket(desock.AF_INET6, desock.SOCK_STREAM, 0)
        desock.connect(s, None, 0)
        assert(desock.recvfrom(s, data, 128, 0, sockaddr, sockaddr_len) == 5)
        desock.close(s)
        assert(data.value == b"12345")
        assert(sockaddr.sin6_family == desock.AF_INET6)
        assert(sockaddr.sin6_port == 53764)
        assert(sockaddr.sin6_flowinfo == 0)
        assert(sockaddr.sin6_scope_id == 0)
        assert(bytes(sockaddr.sin6_addr) == bytes([0] * 15 + [1]))

def test_readv_with_data():
    n = None
    bufs = 1
    buf_len = 1
    iov = helper.create_iovec(bufs, buf_len)
    input = [b"Y"]
    
    with helper.StdinPipe() as pipe:
        pipe.write(b"".join(input))
        fd = desock._debug_instant_fd(0)
        n = desock.readv(fd, iov, bufs)
        
    assert(n == bufs * buf_len)
    
    for i in range(bufs):
        assert(iov[i].iov_len == buf_len)
        assert(iov[i].iov_base[:buf_len] == input[i])
        
def test_readv_without_data():
    iov = helper.create_iovec(1, 0)
    fd = desock._debug_instant_fd(0)
    n = desock.readv(fd, iov, 1)
    assert(n == 0)
    
def test_readv_without_desock():
    data = b"test123"
    
    def handle_connection(fd):
        nonlocal data
        iov = helper.create_iovec(1, len(data))
        n = desock.readv(fd, iov, 1)
        assert(n == len(data))
        assert(iov[0].iov_base[:len(data)] == data)
        
    helper.interact_with_real_server(handle_connection, data)

def test_recvmsg_with_data():
    n = None
    bufs = 10
    buf_len = 10
    opt_buf_len = 50
    input = []
    
    for i in range(bufs):
        input.append(bytes([65 + i] * buf_len))
    
    iovs = helper.create_iovec(bufs, buf_len)
    msghdr = helper.create_msghdr(iov=iovs)
    
    with helper.StdinPipe() as pipe:
        pipe.write(b"".join(input))
        fd = desock._debug_instant_fd(0)
        n = desock.recvmsg(fd, msghdr, 0)
    
    assert(n == bufs * buf_len)
    assert(msghdr.msg_iovlen == bufs)
    assert(msghdr.msg_flags == 0)
    
    for i in range(bufs):
        assert(msghdr.msg_iov[i].iov_len == buf_len)
        assert(msghdr.msg_iov[i].iov_base[:buf_len] == input[i])
        
def test_recvmsg_without_data():
    msghdr = helper.create_msghdr()
    fd = desock._debug_instant_fd(0)
    n = desock.recvmsg(fd, msghdr, 0)
    assert(n == 0)

def test_recvmsg_without_desock():
    data = b"test123"
    
    def handle_connection(fd):
        nonlocal data
        iov = helper.create_iovec(1, len(data))
        msghdr = helper.create_msghdr(iov=iov)
        n = desock.recvmsg(fd, msghdr, 0)
        assert(n == len(data))
        assert(msghdr.msg_iov[0].iov_base[:len(data)] == data)
        
    helper.interact_with_real_server(handle_connection, data)
    
def test_recvmmsg_with_data():
    entries = 5
    bufs = 10
    buf_len = 2
    opt_buf_len = 50
    n = None
    input = []
    tmp = []
    
    for i in range(entries * bufs):
        input.append(bytes([65 + i] * buf_len))
    
    for i in range(entries):
        iov = helper.create_iovec(bufs, buf_len)
        msghdr = helper.create_msghdr(iov=iov)
        tmp.append(helper.create_mmsghdr(msghdr))
    
    mmsghdrs = (desock.mmsghdr * entries)(*tmp)
    
    with helper.StdinPipe() as pipe:
        pipe.write(b"".join(input))
        fd = desock._debug_instant_fd(0)
        n = desock.recvmmsg(fd, mmsghdrs, entries, 0, None)
    
    assert(n == entries)
    
    for i in range(entries):
        assert(mmsghdrs[i].msg_len == bufs * buf_len)
        
        assert(mmsghdrs[i].msg_hdr.msg_iovlen == bufs)
        assert(mmsghdrs[i].msg_hdr.msg_flags == 0)
        
        for j in range(bufs):
            assert(mmsghdrs[i].msg_hdr.msg_iov[j].iov_len == buf_len)
            assert(mmsghdrs[i].msg_hdr.msg_iov[j].iov_base[:buf_len] == input.pop(0))
            
    assert(len(input) == 0)

def test_recvmmsg_without_data():
    mmsghdr = helper.create_mmsghdr(helper.create_msghdr())
    fd = desock._debug_instant_fd(0)
    n = desock.recvmmsg(fd, mmsghdr, 0, 0, None)
    assert(n == 0)
    assert(mmsghdr.msg_len == 0)
    
def test_recvmmsg_without_desock():
    data = b"test123"
    
    def handle_connection(fd):
        nonlocal data
        iov = helper.create_iovec(1, len(data))
        msghdr = helper.create_msghdr(iov=iov)
        mmsghdr = helper.create_mmsghdr(msghdr)
        n = desock.recvmmsg(fd, mmsghdr, 1, 0, None)
        assert(n == 1)
        assert(mmsghdr.msg_hdr.msg_iov[0].iov_base[:len(data)] == data)
        
    helper.interact_with_real_server(handle_connection, data)

def test_recvmsg_name():
    iov = helper.create_iovec(1, 16)
    namelen = ctypes.sizeof(desock.sockaddr_in())
    msg = helper.create_msghdr(iov=iov, namelen=namelen)
    s, c = helper.get_fake_connection(desock.AF_INET, desock.SOCK_STREAM)
    
    with helper.StdinPipe() as pipe:
        pipe.close()
        assert(desock.recvmsg(c, msg, 0) == 0)
    
    assert(msg.msg_namelen == namelen)
    name = bytes(msg.msg_name[:namelen])
    sin_family = int.from_bytes(name[:2], "little")
    assert(sin_family == desock.AF_INET)
    sin_port = int.from_bytes(name[2:4], "little")
    assert(sin_port == 53764)
    sin_addr = int.from_bytes(name[4:8], "little")
    assert(sin_addr == 0x100007f)
    
    desock.close(s)
    desock.close(c)
