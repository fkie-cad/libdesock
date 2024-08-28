#define _GNU_SOURCE
#define __USE_GNU
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>

#include "util.h"
#include "syscall.h"
#include "desock.h"
#include "hooks.h"

static ssize_t do_writev (const struct iovec* iov, int len) {
    ssize_t written = 0;

    for (int i = 0; i < len; ++i) {
        size_t offset = 0;
        ssize_t r;

        do {
            r = hook_output((char *) iov[i].iov_base + offset, iov[i].iov_len - offset);

            if (UNLIKELY(r == -1)) {
                return -1;
            }

            written += r;
            offset += r;
        } while (offset < iov[i].iov_len && r > 0);
    }

    return written;
}

VISIBLE
ssize_t write (int fd, const void* buf, size_t count) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_write, fd, buf, count);
    }
    
    DEBUG_LOG("write(%d, %p, %lu)", fd, buf, count);
    ssize_t r = hook_output(buf, count);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(write)

VISIBLE
ssize_t sendto (int fd, const void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t alen) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(sendto, fd, buf, len, flags, addr, alen);
    }
    
    DEBUG_LOG("sendto(%d, %p, %lu, %d, %p, %lu)", fd, buf, len, flags, addr, alen);    
    ssize_t r = hook_output(buf, len);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(sendto)

VISIBLE
ssize_t send (int fd, const void* buf, size_t len, int flags) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(sendto, fd, buf, len, flags, 0, 0);
    }
    
    DEBUG_LOG("send(%d, %p, %lu, %d)", fd, buf, len, flags);    
    ssize_t r = hook_output(buf, len);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(send)

VISIBLE
ssize_t sendmsg (int fd, const struct msghdr* msg, int flags) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall_cp(sendmsg, fd, msg, flags, 0, 0, 0);
    }
    
    DEBUG_LOG("sendmsg(%d, %p, %d)", fd, msg, flags);    
    ssize_t r = do_writev(msg->msg_iov, msg->msg_iovlen);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(sendmsg)

VISIBLE
int sendmmsg (int fd, struct mmsghdr* msgvec, unsigned int vlen, int flags) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_sendmmsg, fd, msgvec, vlen, flags);
    }
    
    DEBUG_LOG("sendmmsg(%d, %p, %d, %d)", fd, msgvec, vlen, flags);

    unsigned int i;
    for (i = 0; i < vlen; ++i) {
        ssize_t r = do_writev(msgvec[i].msg_hdr.msg_iov, msgvec[i].msg_hdr.msg_iovlen);
        
        if (r == -1) {
            return -1;
        }
        
        msgvec[i].msg_len = r;
    }

    ssize_t r = (i == vlen || i == 0) ? i : (i + 1);
    DEBUG_LOG(" => %ld", r);
    return r;
}
VERSION(sendmmsg)

VISIBLE
ssize_t writev (int fd, const struct iovec* iov, int count) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return syscall_cp(SYS_writev, fd, iov, count);
    }
    
    DEBUG_LOG("writev(%d, %p, %d)", fd, iov, count);
    ssize_t r = do_writev(iov, count);
    DEBUG_LOG(" => %lu", r);
    return r;
}
VERSION(writev)
