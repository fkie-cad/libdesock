#define _GNU_SOURCE
#define __USE_GNU
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>

#include "util.h"
#include "syscall.h"
#include "desock.h"
#include "peekbuffer.h"
#include "hooks.h"
#include "multi.h"
#include "stub_sockaddr.h"

static ssize_t do_readv (struct iovec* iov, int len, int* full, int peek, int offset) {
    size_t read_in = 0;

    if (full) {
        *full = 1;
    }

    for (int i = 0; i < len; ++i) {
        size_t r = 0;

        if (peek) {
            r = peekbuffer_cp(iov[i].iov_base, iov[i].iov_len, offset);
            offset += r;
        } else {
            char was_locked = 0;
            
            if (peekbuffer_size() > 0) {
                was_locked = peekbuffer_locked();
                
                r = peekbuffer_mv(iov[i].iov_base, iov[i].iov_len);
            }

            if (!was_locked && r < iov[i].iov_len) {
                ssize_t n = hook_input((char *) iov[i].iov_base + r, iov[i].iov_len - r);
                
                if (UNLIKELY(n < 0)) {
                    return -1;
                }

                n = postprocess_input((char *) iov[i].iov_base + r, n);
                r += n;
            }
        }

        read_in += r;

        if (r < iov[i].iov_len) {
            if (full) {
                *full = 0;
            }

            break;
        }
    }

    return read_in;
}

static ssize_t do_recv (char* buf, size_t len, int peek) {
    size_t buflen = peekbuffer_size();
    size_t offset = 0;

    if (peek) {
        long delta = len - buflen;

        if (UNLIKELY(delta > 0 && peekbuffer_read(delta) == -1)) {
            return -1;
        }

        return peekbuffer_cp(buf, len, 0);
    }
    
    if (buflen > 0) {
        char was_locked = peekbuffer_locked();

        offset = peekbuffer_mv(buf, len);

        if (was_locked) {
            return offset;
        }
    }

    if (offset < len) {
        ssize_t n = hook_input(buf + offset, len - offset);

        if (UNLIKELY(n < 0)) {
            return -1;
        }
        
        n = postprocess_input(buf + offset, n);
        offset += n;
    }

    return offset;
}

VISIBLE
ssize_t read (int fd, void* buf, size_t count) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_read, fd, buf, count);
    }
    
    DEBUG_LOG("read(%d, %p, %lu)", fd, buf, count);

    size_t offset = 0;

    if (peekbuffer_size() > 0) {
        offset = peekbuffer_mv(buf, count);
    }

    if (offset < count) {
        ssize_t n = hook_input((char *) buf + offset, count - offset);

        if (UNLIKELY(n < 0)) {
            return -1;
        }
        
        n = postprocess_input((char *) buf + offset, n);
        offset += n;
    }

    DEBUG_LOG(" => %ld", offset);
    return offset;
}
VERSION(read)

VISIBLE
ssize_t recvfrom (int fd, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* alen) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(recvfrom, fd, buf, len, flags, addr, alen);
    }
    
    DEBUG_LOG("recvfrom(%d, %p, %lu, %d, %p, %p)", fd, buf, len, flags, addr, alen);

    fill_sockaddr(fd, addr, alen);

    ssize_t r = do_recv(buf, len, flags & MSG_PEEK);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(recvfrom)

VISIBLE
ssize_t recv (int fd, void* buf, size_t len, int flags) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(recvfrom, fd, buf, len, flags, NULL, NULL);
    }
    
    DEBUG_LOG("recv(%d, %p, %lu, %d)", fd, buf, len, flags);
    ssize_t r = do_recv(buf, len, flags & MSG_PEEK);
    DEBUG_LOG(" => %d", r);
    return r;
}
VERSION(recv)

VISIBLE
ssize_t recvmsg (int fd, struct msghdr* msg, int flags) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(recvmsg, fd, msg, flags, 0, 0, 0);
    }
    
    DEBUG_LOG("recvmsg(%d, %p, %d)", fd, msg, flags);
    
    int peek = flags & MSG_PEEK;

    if (peek) {
        size_t total_length = 0;

        for (size_t i = 0; i < msg->msg_iovlen; ++i) {
            total_length += msg->msg_iov[i].iov_len;
        }

        ssize_t delta = total_length - peekbuffer_size();

        if (UNLIKELY(delta > 0 && peekbuffer_read(delta) == -1)) {
            return -1;
        }
    }

    msg->msg_flags = 0;

    fill_sockaddr(fd, msg->msg_name, &msg->msg_namelen);

    ssize_t r = do_readv(msg->msg_iov, msg->msg_iovlen, NULL, peek, 0);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(recvmsg)

VISIBLE
int recvmmsg (int fd, struct mmsghdr* msgvec, unsigned int vlen, int flags, struct timespec* timeout) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_recvmmsg, fd, msgvec, vlen, flags, timeout);
    }
    
    DEBUG_LOG("recvmmsg(%d, %p, %d, %d, %p)", fd, msgvec, vlen, flags, timeout);

    unsigned int i;
    ssize_t offset = 0;
    ssize_t r;
    int peek = flags & MSG_PEEK;

    if (peek) {
        size_t total_length = 0;

        for (i = 0; i < vlen; ++i) {
            for (size_t j = 0; j < msgvec[i].msg_hdr.msg_iovlen; ++j) {
                total_length += msgvec[i].msg_hdr.msg_iov[j].iov_len;
            }
        }

        ssize_t delta = total_length - peekbuffer_size();

        if (UNLIKELY(delta > 0 && peekbuffer_read(delta) == -1)) {
            return -1;
        }
    }

    for (i = 0; i < vlen; ++i) {
        int full = 0;

        msgvec[i].msg_hdr.msg_flags = 0;

        fill_sockaddr(fd, msgvec[i].msg_hdr.msg_name, &msgvec[i].msg_hdr.msg_namelen);

        r = do_readv(msgvec[i].msg_hdr.msg_iov, msgvec[i].msg_hdr.msg_iovlen, &full, peek, offset);
        
        if (UNLIKELY(r < 0)) {
            return -1;
        }
        
        msgvec[i].msg_len = (unsigned int) r;
        offset += r;

        if (!full) {
            break;
        }
    }

    r = (i == vlen || i == 0) ? i : (i + 1);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(recvmmsg)

VISIBLE
ssize_t readv (int fd, struct iovec* iov, int count) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_readv, fd, iov, count);
    }
    
    DEBUG_LOG("readv(%d, %p, %d)", fd, iov, count);
    int r = do_readv(iov, count, NULL, 0, 0);
    DEBUG_LOG(" => %d", r);
    return r;
}
VERSION(readv)
