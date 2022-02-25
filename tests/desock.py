import os
import socket
import ctypes
import faulthandler
faulthandler.enable()

FD_TABLE_SIZE = 512
STATIC_BUFFER_SIZE = 8192

class DesockError(RuntimeError):
    pass

class sockaddr(ctypes.Structure):
    _fields_ = [
        ("sa_family", ctypes.c_ushort),
        ("sa_data", ctypes.c_char * 14)
    ]
    
class sockaddr_in(ctypes.Structure):
    _fields_ = [
        ("sin_family", ctypes.c_ushort),
        ("sin_port", ctypes.c_ushort),
        ("sin_addr", ctypes.c_uint),
        ("sin_zero", ctypes.c_char * 8)
    ]
    
class sockaddr_in6(ctypes.Structure):
    _fields_ = [
        ("sin6_family", ctypes.c_ushort),
        ("sin6_port", ctypes.c_ushort),
        ("sin6_flowinfo", ctypes.c_uint),
        ("sin6_addr", ctypes.c_byte * 16),
        ("sin6_scope_id", ctypes.c_uint)
    ]
    
class sockaddr_un(ctypes.Structure):
    _fields_ = [
        ("sun_family", ctypes.c_ushort),
        ("sun_path", ctypes.c_byte * 108)
    ]

class iovec(ctypes.Structure):
    _fields_ = [
        ("iov_base", ctypes.POINTER(ctypes.c_char)),
        ("iov_len", ctypes.c_size_t)
    ]
    
class msghdr(ctypes.Structure):
    _fields_ = [
        ("msg_name", ctypes.POINTER(ctypes.c_char)),
        ("msg_namelen", ctypes.c_uint),
        ("msg_iov", ctypes.POINTER(iovec)),
        ("msg_iovlen", ctypes.c_size_t),
        ("msg_control", ctypes.POINTER(ctypes.c_char)),
        ("msg_controllen", ctypes.c_size_t),
        ("msg_flags", ctypes.c_int)
    ]

class mmsghdr(ctypes.Structure):
    _fields_ = [
        ("msg_hdr", msghdr),
        ("msg_len", ctypes.c_uint)
    ]
    
FD_SETSIZE = 1024
    
class fd_set(ctypes.Structure):
    _fields_ = [
        ("fds_bits", ctypes.c_long * (FD_SETSIZE // 64))
    ]
    
def FD_ZERO(s):
    for i in range(len(s.fds_bits)):
        s.fds_bits[i] = 0
        
def FD_SET(fd, s):
    assert(0 <= fd < FD_SETSIZE)
    s.fds_bits[fd // 64] |= (1 << (fd % 64))
    
def FD_CLR(fd, s):
    assert(0 <= fd < FD_SETSIZE)
    s.fds_bits[fd // 64] &= ~(1 << (fd % 64))
    
def FD_ISSET(fd, s):
    assert(0 <= fd < FD_SETSIZE)
    return (s.fds_bits[fd // 64] & (1 << (fd % 64))) != 0
    
def FD_COPY(dst, src):
    for i in range(len(dst.fds_bits)):
        dst.fds_bits[i] = src.fds_bits[i]

class pollfd(ctypes.Structure):
    _fields_ = [
        ("fd", ctypes.c_int),
        ("events", ctypes.c_short),
        ("revents", ctypes.c_short)
    ]
    
class epoll_data(ctypes.Union):
    _fields_ = [
        ("ptr", ctypes.c_void_p),
        ("fd", ctypes.c_int),
        ("u32", ctypes.c_uint),
        ("u64", ctypes.c_ulonglong)
    ]
    
class epoll_event(ctypes.Structure):
    _fields_ = [
        ("events", ctypes.c_uint),
        ("data", epoll_data)
    ]
    
class fd_entry(ctypes.Structure):
    _fields_ = [
        ("domain", ctypes.c_int),
        ("desock", ctypes.c_int),
        ("listening", ctypes.c_int),
        ("epfd", ctypes.c_int),
        ("ep_event", epoll_event)
    ]

MSG_PEEK = socket.MSG_PEEK.value
AF_INET = socket.AF_INET.value
AF_INET6 = socket.AF_INET6.value
SOCK_STREAM = socket.SOCK_STREAM.value
SOCK_DGRAM = socket.SOCK_DGRAM.value
POLLIN = 1
POLLOUT = 4
POLLERR = 8
POLLHUP = 16
POLLNVAL = 32
EPOLL_CTL_ADD = 1
EPOLL_CTL_DEL = 2
EPOLL_CTL_MOD = 3
EPOLLIN = 1
EPOLLOUT = 4
EPOLLERR = 8
EPOLLHUP = 16
EPOLLONESHOT = 1 << 30

_libdesock = ctypes.cdll.LoadLibrary(os.getenv("LIBDESOCK"))

pure_debug_functions = ["_debug_instant_fd", "_debug_real_bind", "_debug_real_dup2", "_debug_get_fd_table_entry"]
for debug_fn in pure_debug_functions:
    if not hasattr(_libdesock, debug_fn):
        print("It looks like you are not testing the debug-build of the library!")
        exit(1)

_libc = ctypes.CDLL("libc.so.6")

_libc.__errno_location.restype = ctypes.POINTER(ctypes.c_int)
_libc.strerror.restype = ctypes.c_char_p
def _get_error():
    return _libc.strerror(_libc.__errno_location()[0]).decode()
    
def _clear_error():
    _libc.__errno_location().contents = ctypes.c_int(0)

_libdesock.socket.restype = ctypes.c_int
def socket(domain, type, protocol):
    _clear_error()
    r = _libdesock.socket(
        ctypes.c_int(domain),
        ctypes.c_int(type),
        ctypes.c_int(protocol)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.close.restype = ctypes.c_int
def close(fd):
    _clear_error()
    r = _libdesock.close(
        ctypes.c_int(fd)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r

_libdesock.accept.restype = ctypes.c_int
def accept(fd, addr, len):
    if addr is not None:
        addr = ctypes.byref(addr)
    if len is not None:
        len = ctypes.byref(len)
        
    _clear_error()
    r = _libdesock.accept(
        ctypes.c_int(fd),
        addr,
        len
    )
    if r < 0:
        raise DesockError(_get_error())
    return r

_libdesock.accept4.restype = ctypes.c_int
def accept4(fd, addr, len, flag=0):
    if addr is not None:
        addr = ctypes.byref(addr)
    if len is not None:
        len = ctypes.byref(len)
    
    _clear_error()
    r = _libdesock.accept4(
        ctypes.c_int(fd),
        addr,
        len,
        ctypes.c_int(flag)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.bind.restype = ctypes.c_int
def bind(fd, addr, len):
    if addr is not None:
        addr = ctypes.byref(addr)
    
    _clear_error()
    r = _libdesock.bind(
        ctypes.c_int(fd),
        addr,
        ctypes.c_int(len)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.listen.restype = ctypes.c_int
def listen(fd, backlog):
    _clear_error()
    r = _libdesock.listen(
        ctypes.c_int(fd),
        ctypes.c_int(backlog)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.dup.restype = ctypes.c_int
def dup(fd):
    _clear_error()
    r = _libdesock.dup(ctypes.c_int(fd))
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.dup2.restype = ctypes.c_int
def dup2(old, new):
    _clear_error()
    r = _libdesock.dup2(
        ctypes.c_int(old),
        ctypes.c_int(new)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.dup3.restype = ctypes.c_int
def dup3(old, new, flags):
    _clear_error()
    r = _libdesock.dup3(
        ctypes.c_int(old),
        ctypes.c_int(new),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.read.restype = ctypes.c_int
def read(fd, buf, count):
    _clear_error()
    r = _libdesock.read(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_int(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.recv.restype = ctypes.c_int
def recv(fd, buf, len, flags):
    _clear_error()
    r = _libdesock.recv(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_int(len),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.recvfrom.restype = ctypes.c_int
def recvfrom(fd, buf, len, flags, addr, alen):
    if addr is not None:
        addr = ctypes.byref(addr)
    if alen is not None:
        alen = ctypes.byref(alen)
        
    _clear_error()
    r = _libdesock.recvfrom(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_int(len),
        ctypes.c_int(flags),
        addr,
        alen
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.recvmsg.restype = ctypes.c_int
def recvmsg(fd, msg, flags):
    _clear_error()
    r = _libdesock.recvmsg(
        ctypes.c_int(fd),
        ctypes.byref(msg),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.recvmmsg.restype = ctypes.c_int
def recvmmsg(fd, msgvec, vlen, flags, timeout):
    if timeout is not None:
        timeout = ctypes.byref(timeout)
    
    _clear_error()
    r = _libdesock.recvmmsg(
        ctypes.c_int(fd),
        ctypes.byref(msgvec),
        ctypes.c_int(vlen),
        ctypes.c_int(flags),
        timeout
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.readv.restype = ctypes.c_int
def readv(fd, iov, count):
    _clear_error()
    r = _libdesock.readv(
        ctypes.c_int(fd),
        ctypes.byref(iov),
        ctypes.c_int(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock._debug_instant_fd.restype = ctypes.c_int
def _debug_instant_fd(listening):
    _clear_error()
    r = _libdesock._debug_instant_fd(
        ctypes.c_int(listening)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock._debug_real_bind.restype = ctypes.c_int
def _debug_real_bind(fd, addr, len):
    if addr is not None:
        addr = ctypes.byref(addr)
    
    _clear_error()
    r = _libdesock._debug_real_bind(
        ctypes.c_int(fd),
        addr,
        ctypes.c_int(len)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.write.restype = ctypes.c_ssize_t
def write(fd, buf, count):
    _clear_error()
    r = _libdesock.write(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_size_t(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.send.restype = ctypes.c_ssize_t
def send(fd, buf, len, flags):
    _clear_error()
    r = _libdesock.send(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_int(len),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.sendto.restype = ctypes.c_ssize_t
def sendto(fd, buf, len, flags, addr, alen):
    if addr is not None:
        addr = ctypes.byref(addr)
        
    _clear_error()
    r = _libdesock.sendto(
        ctypes.c_int(fd),
        ctypes.byref(buf),
        ctypes.c_int(len),
        ctypes.c_int(flags),
        addr,
        ctypes.c_int(alen)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.sendmsg.restype = ctypes.c_ssize_t
def sendmsg(fd, msg, flags):
    _clear_error()
    r = _libdesock.sendmsg(
        ctypes.c_int(fd),
        ctypes.byref(msg),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r

_libdesock.sendmmsg.restype = ctypes.c_int
def sendmmsg(fd, msgvec, vlen, flags):
    _clear_error()
    r = _libdesock.sendmmsg(
        ctypes.c_int(fd),
        None if msgvec is None else ctypes.byref(msgvec),
        ctypes.c_int(vlen),
        ctypes.c_int(flags)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.writev.restype = ctypes.c_ssize_t
def writev(fd, iov, count):
    _clear_error()
    r = _libdesock.writev(
        ctypes.c_int(fd),
        ctypes.byref(iov),
        ctypes.c_int(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.select.restype = ctypes.c_int
def select(nfds, rfds, wfds, efds, timeout):
    if rfds is not None:
        rfds = ctypes.byref(rfds)
    if wfds is not None:
        wfds = ctypes.byref(wfds)
    if efds is not None:
        efds = ctypes.byref(efds)
    if timeout is not None:
        timeout = ctypes.byref(timeout)
    
    _clear_error()
    r = _libdesock.select(
        ctypes.c_int(nfds),
        rfds,
        wfds,
        efds,
        timeout
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.pselect.restype = ctypes.c_int
def pselect(nfds, rfds, wfds, efds, timeout, sigmask):
    if rfds is not None:
        rfds = ctypes.byref(rfds)
    if wfds is not None:
        wfds = ctypes.byref(wfds)
    if efds is not None:
        efds = ctypes.byref(efds)
    if timeout is not None:
        timeout = ctypes.byref(timeout)
    if sigmask is not None:
        sigmask = ctypes.byref(sigmask)
    
    _clear_error()
    r = _libdesock.pselect(
        ctypes.c_int(nfds),
        rfds,
        wfds,
        efds,
        timeout,
        sigmask
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.poll.restype = ctypes.c_int
def poll(fds, nfds, timeout):
    if fds is not None:
        fds = ctypes.byref(fds)
    
    _clear_error()
    r = _libdesock.poll(
        fds,
        ctypes.c_int(nfds),
        ctypes.c_int(timeout)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.ppoll.restype = ctypes.c_int
def ppoll(fds, nfds, timeout, mask):
    if fds is not None:
        fds = ctypes.byref(fds)
    if timeout is not None:
        timeout = ctypes.byref(timeout)
    if mask is not None:
        mask = ctypes.byref(mask)
    
    _clear_error()
    r = _libdesock.ppoll(
        fds,
        ctypes.c_int(nfds),
        timeout,
        mask
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.epoll_create.restype = ctypes.c_int
def epoll_create():
    _clear_error()
    r = _libdesock.epoll_create(0)
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.epoll_ctl.restype = ctypes.c_int
def epoll_ctl(epfd, op, fd, event):
    if event is not None:
        event = ctypes.byref(event)
    
    _clear_error()
    r = _libdesock.epoll_ctl(
        ctypes.c_int(epfd),
        ctypes.c_int(op),
        ctypes.c_int(fd),
        event
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.epoll_wait.restype = ctypes.c_int
def epoll_wait(epfd, events, maxevents, timeout):
    _clear_error()
    r = _libdesock.epoll_wait(
        ctypes.c_int(epfd),
        ctypes.byref(events),
        ctypes.c_int(maxevents),
        ctypes.c_int(timeout)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.epoll_pwait.restype = ctypes.c_int
def epoll_pwait(epfd, events, maxevents, timeout, sigmask):
    if sigmask is not None:
        sigmask = ctypes.byref(sigmask)
    
    _clear_error()
    r = _libdesock.epoll_pwait(
        ctypes.c_int(epfd),
        ctypes.byref(events),
        ctypes.c_int(maxevents),
        ctypes.c_int(timeout),
        sigmask
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.epoll_pwait2.restype = ctypes.c_int
def epoll_pwait2(epfd, events, maxevents, timeout, sigmask):
    if timeout is not None:
        timeout = ctypes.byref(timeout)
    if sigmask is not None:
        sigmask = ctypes.byref(sigmask)
    
    _clear_error()
    r = _libdesock.epoll_pwait2(
        ctypes.c_int(epfd),
        ctypes.byref(events),
        ctypes.c_int(maxevents),
        timeout,
        sigmask
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.getpeername.restype = ctypes.c_int
def getpeername(sockfd, addr, addrlen):
    _clear_error()
    r = _libdesock.getpeername(
        ctypes.c_int(sockfd),
        ctypes.byref(addr),
        ctypes.byref(addrlen)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.getsockname.restype = ctypes.c_int
def getsockname(sockfd, addr, addrlen):
    _clear_error()
    r = _libdesock.getsockname(
        ctypes.c_int(sockfd),
        ctypes.byref(addr),
        ctypes.byref(addrlen)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.sendfile.restype = ctypes.c_int
def sendfile(out_fd, in_fd, offset, count):
    if offset is not None:
        offset = ctypes.byref(offset)
    
    _clear_error()
    r = _libdesock.sendfile(
        ctypes.c_int(out_fd),
        ctypes.c_int(in_fd),
        offset,
        ctypes.c_size_t(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r

_libdesock.sendfile64.restype = ctypes.c_int
def sendfile64(out_fd, in_fd, offset, count):
    if offset is not None:
        offset = ctypes.byref(offset)

    _clear_error()
    r = _libdesock.sendfile64(
        ctypes.c_int(out_fd),
        ctypes.c_int(in_fd),
        offset,
        ctypes.c_size_t(count)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r

_libdesock._debug_real_dup2.restype = ctypes.c_int
def _debug_real_dup2(old, new):
    _clear_error()
    r = _libdesock._debug_real_dup2(
        ctypes.c_int(old),
        ctypes.c_int(new)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
_libdesock.connect.restype = ctypes.c_int
def connect(fd, addr, addrlen):
    if addr is not None:
        addr = ctypes.byref(addr)
    
    _clear_error()
    r = _libdesock.connect(
        ctypes.c_int(fd),
        addr,
        ctypes.c_int(addrlen)
    )
    if r < 0:
        raise DesockError(_get_error())
    return r
    
def _debug_get_fd_table_entry(idx, dst):
    _libdesock._debug_get_fd_table_entry(idx, ctypes.byref(dst))
    
def clear_fd_table_entry(idx):
    _libdesock.clear_fd_table_entry(idx)
