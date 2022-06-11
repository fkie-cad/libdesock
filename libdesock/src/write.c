#define _GNU_SOURCE
#define __USE_GNU
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "syscall.h"
#include "desock.h"
#include "hooks.h"

static long internal_writev (const struct iovec* iov, int len) {
    int written = 0;

    for (int i = 0; i < len; ++i) {
        int offset = 0, r;

        do {
            r = hook_output((char *) iov[i].iov_base + offset, iov[i].iov_len - offset);

            if (r == -1) {
                return -1;
            }

            written += r;
            offset += r;
        } while (offset < iov[i].iov_len && r > 0);
    }

    return written;
}

visible ssize_t write (int fd, const void* buf, size_t count) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        int r = hook_output(buf, count);
        DEBUG_LOG ("[%d] desock::write(%d, %p, %lu) = %d\n", gettid (), fd, buf, count, r);
        return r;
    } else {
        return syscall_cp (SYS_write, fd, buf, count);
    }
}

visible ssize_t send (int fd, const void* buf, size_t len, int flags) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        int r = hook_output(buf, len);
        DEBUG_LOG ("[%d] desock::send(%d, %p, %lu, %d) = %d\n", gettid (), fd, buf, len, flags, r);
        return r;
    } else {
        return sendto (fd, buf, len, flags, 0, 0);
    }
}

visible ssize_t sendto (int fd, const void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t alen) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        int r = hook_output(buf, len);
        DEBUG_LOG ("[%d] desock::sendto(%d, %p, %lu, %d, %p, %lu) = %d\n", gettid (), fd, buf, len, flags, addr, alen, r);
        return r;
    } else {
        return socketcall_cp (sendto, fd, buf, len, flags, addr, alen);
    }
}

visible ssize_t sendmsg (int fd, const struct msghdr* msg, int flags) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        int r = internal_writev (msg->msg_iov, msg->msg_iovlen);
        DEBUG_LOG ("[%d] desock::sendmsg(%d, %p, %d) = %d\n", gettid (), fd, msg, flags, r);
        return r;
    } else {
        return socketcall_cp (sendmsg, fd, msg, flags, 0, 0, 0);
    }
}

visible int sendmmsg (int fd, struct mmsghdr* msgvec, unsigned int vlen, int flags) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        DEBUG_LOG ("[%d] desock::sendmmsg(%d, %p, %d, %d)", gettid (), fd, msgvec, vlen, flags);

        int i;

        for (i = 0; i < vlen; ++i) {
            msgvec[i].msg_len = internal_writev (msgvec[i].msg_hdr.msg_iov, msgvec[i].msg_hdr.msg_iovlen);

            if (msgvec[i].msg_len == -1) {
                DEBUG_LOG (" = -1\n");
                return -1;
            }
        }

        int r = (i == vlen || i == 0) ? i : (i + 1);
        DEBUG_LOG (" = %d\n", r);
        return r;
    } else {
        return syscall_cp (SYS_sendmmsg, fd, msgvec, vlen, flags);
    }
}

visible ssize_t writev (int fd, const struct iovec* iov, int count) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        int r = internal_writev (iov, count);
        DEBUG_LOG ("[%d] desock::writev(%d, %p, %d) = %d\n", gettid (), fd, iov, count, r);
        return r;
    } else {
        return syscall_cp (SYS_writev, fd, iov, count);
    }
}
