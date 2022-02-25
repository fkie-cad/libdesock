"""
This file tests recv*() calls with the MSG_PEEK flag set.

Target files:
    - libdesock/src/read.c
"""

import ctypes

import desock
import helper

def test_recvmsg():
    fd = desock._debug_instant_fd(0)
    
    with helper.StdinPipe() as pipe:
        data = bytes(range(128))
        pipe.write(data)
        
        msghdr = helper.create_msghdr(iov=helper.create_iovec(4, 16))
        
        assert(desock.recvmsg(fd, msghdr, desock.MSG_PEEK) == 64)
        
        msghdr = helper.create_msghdr(iov=helper.create_iovec(4, 32))
        assert(desock.recvmsg(fd, msghdr, desock.MSG_PEEK) == 128)
        for i in range(4):
            assert(bytes(msghdr.msg_iov[i].iov_base[:msghdr.msg_iov[i].iov_len]) == data[i * 32 : (i + 1) * 32])
        
        msghdr = helper.create_msghdr(iov=helper.create_iovec(4, 32))
        assert(desock.recvmsg(fd, msghdr, 0) == 128)
        for i in range(4):
            assert(bytes(msghdr.msg_iov[i].iov_base[:msghdr.msg_iov[i].iov_len]) == data[i * 32 : (i + 1) * 32])
    
        pipe.close()
          
        assert(desock.recvmsg(fd, msghdr, 0) == 0)

def test_with_recv_and_read():
    buf = ctypes.create_string_buffer(128)
    fd = desock._debug_instant_fd(0)
    
    with helper.StdinPipe() as pipe:
        data = bytes(range(128))
        pipe.write(data)
        
        assert(desock.recv(fd, buf, 64, desock.MSG_PEEK) == 64)
        assert(desock.recv(fd, buf, 128, desock.MSG_PEEK) == 128)
        assert(buf.raw == data)
        
        buf = ctypes.create_string_buffer(128)
        assert(desock.recv(fd, buf, 128, 0) == 128)
        assert(buf.raw == data)
    
        pipe.close()
          
        assert(desock.recv(fd, buf, 128, 0) == 0)
    
    with helper.StdinPipe() as pipe:
        data = bytes(range(128, 256))
        pipe.write(data)
        
        assert(desock.recv(fd, buf, 128, desock.MSG_PEEK) == 128)
        
        for i in range(0, 64, 2):
            assert(desock.recv(fd, buf, 2, desock.MSG_PEEK) == 2)
            assert(buf[:2] == data[i:i+2])
            assert(desock.recv(fd, buf, 2, 0) == 2)
            assert(buf[:2] == data[i:i+2])
            
        buf = ctypes.create_string_buffer(128)
        data = bytes([1] * 64)
        pipe.write(data)
        
        assert(desock.recv(fd, buf, 128, desock.MSG_PEEK) == 128)
        assert(buf.raw[:64] == bytes(range(128 + 64, 256)))
        assert(buf.raw[64:] == data)
        
        assert(desock.read(fd, buf, 64) == 64)
        assert(buf.raw[:64] == bytes(range(128 + 64, 256)))
        assert(desock.read(fd, buf, 64) == 64)
        assert(buf.raw[:64] == data)
        pipe.close()
        assert(desock.read(fd, buf, 64) == 0)

def _collect_data(mmsghdrs):
    ret = b""
    for i in range(len(mmsghdrs)):
        tmp = b""
        for j in range(mmsghdrs[i].msg_hdr.msg_iovlen):
            tmp += mmsghdrs[i].msg_hdr.msg_iov[j].iov_base[:mmsghdrs[i].msg_hdr.msg_iov[j].iov_len]
        ret += tmp[:mmsghdrs[i].msg_len]
    return ret
    
def _collect_len(mmsghdrs):
    ret = 0
    for i in range(len(mmsghdrs)):
        ret += mmsghdrs[i].msg_len
    return ret
    
def _new_struct(n=4):
    mmsghdrs = (desock.mmsghdr * n)()
    for i in range(n):
        mmsghdrs[i] = helper.create_mmsghdr(helper.create_msghdr(iov=helper.create_iovec(5, 5)))
    return mmsghdrs

def test_with_recvmmsg_and_readv():
    mmsghdrs = _new_struct()
    fd = desock._debug_instant_fd(0)
    
    with helper.StdinPipe() as pipe:
        data = bytes(range(100))
        pipe.write(data)
        
        assert(desock.recvmmsg(fd, mmsghdrs, 2, desock.MSG_PEEK, None) == 2)
        assert(_collect_len(mmsghdrs) == 50)
        assert(desock.recvmmsg(fd, mmsghdrs, 3, desock.MSG_PEEK, None) == 3)
        assert(_collect_len(mmsghdrs) == 75)
        assert(desock.recvmmsg(fd, mmsghdrs, 4, desock.MSG_PEEK, None) == 4)
        assert(_collect_len(mmsghdrs) == 100)
        assert(_collect_data(mmsghdrs) == data)
        
        mmsghdrs = _new_struct()
        assert(desock.recvmmsg(fd, mmsghdrs, 4, 0, None) == 4)
        assert(_collect_len(mmsghdrs))
        assert(_collect_data(mmsghdrs) == data)
        
        pipe.close()
        
        assert(desock.recvmmsg(fd, mmsghdrs, 4, 0, None) == 0)
        
    with helper.StdinPipe() as pipe:
        mmsghdrs = _new_struct()
        data = bytes(range(128, 228))
        pipe.write(data)
        
        assert(desock.recvmmsg(fd, mmsghdrs, 4, desock.MSG_PEEK, None) == 4)
        assert(_collect_len(mmsghdrs) == 100)
        
        for i in range(2):
            mmsghdrs = _new_struct()
            assert(desock.recvmmsg(fd, mmsghdrs, 1, desock.MSG_PEEK, None) == 1)
            assert(_collect_len(mmsghdrs) == 25)
            assert(_collect_data(mmsghdrs) == data[i * 25 : (i + 1) * 25])
            
            assert(desock.recvmmsg(fd, mmsghdrs, 1, 0, None) == 1)
            assert(_collect_len(mmsghdrs) == 25)
            assert(_collect_data(mmsghdrs) == data[i * 25 : (i + 1) * 25])
            
        mmsghdrs = _new_struct(3)
        data = bytes([1] * 25)
        pipe.write(data)
        
        assert(desock.recvmmsg(fd, mmsghdrs, 3, desock.MSG_PEEK, None) == 3)
        assert(_collect_len(mmsghdrs) == 75)
        assert(_collect_data(mmsghdrs) == bytes(range(128, 228))[50:] + data)
        
        data = bytes(range(128, 228))[50:] + data
        for i in range(3):
            iov = helper.create_iovec(5, 5)
            n = desock.readv(fd, iov, 5)
            assert(n == 25)
            
            ret = b""
            for j in range(5):
                ret += iov[j].iov_base[:iov[j].iov_len]
            assert(ret == data[i * 25 : (i + 1) * 25])
            
        pipe.close()
        assert(desock.readv(fd,  helper.create_iovec(5, 5), 5) == 0)

def test_wrap_around():
    fd = desock._debug_instant_fd(0)
    
    with helper.StdinPipe() as pipe:
        for c, _ in enumerate( range(0, desock.STATIC_BUFFER_SIZE, 1024) ):
            buf = ctypes.create_string_buffer(1024 * (c + 1))
            pipe.write(bytes([0x40 + c] * 1024))
            assert(desock.recv(fd, buf, 1024 * (c + 1), desock.MSG_PEEK) == 1024 * (c + 1))
            assert(buf.raw[-1024:] == bytes([0x40 + c] * 1024))
            
        buf_size = desock.STATIC_BUFFER_SIZE - 1
        buf = ctypes.create_string_buffer(buf_size)
        
        assert(desock.recv(fd, buf, buf_size, 0) == buf_size)
        for i in range(desock.STATIC_BUFFER_SIZE // 1024):
            assert(set(buf.raw[1024 * i:1024 * (i + 1)]) == set([0x40 + i]))
            
        pipe.write([0x0A] * 16)
        assert(desock.recv(fd, buf, 17, desock.MSG_PEEK) == 17)
        assert(buf[0][0] == 0x47)
        assert(buf[1:17] == bytes([0x0A] * 16))
        
        pipe.close()
        desock.recv(fd, buf, 4096, 0)
        
def test_move_to_heap():
    fd = desock._debug_instant_fd(0)
    
    with helper.StdinPipe() as pipe:
        for c, _ in enumerate( range(0, desock.STATIC_BUFFER_SIZE * 2 + 1, 1024) ):
            buf = ctypes.create_string_buffer(1024 * (c + 1))
            pipe.write(bytes([0x40 + c] * 1024))
            assert(desock.recv(fd, buf, 1024 * (c + 1), desock.MSG_PEEK) == 1024 * (c + 1))
            
            for i in range(c + 1):
                assert(buf.raw[1024 * i:1024 * (i + 1)] == bytes([0x40 + i] * 1024))
