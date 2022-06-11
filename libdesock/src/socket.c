#define _GNU_SOURCE
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "syscall.h"
#include "desock.h"

visible int socket (int domain, int type, int protocol) {
    DEBUG_LOG ("[%d] desock::socket(%d, %d, %d)", gettid (), domain, type, protocol);

    int s = __socketcall (socket, domain, type, protocol, 0, 0, 0);
    if ((s == -EINVAL || s == -EPROTONOSUPPORT)
        && (type & (SOCK_CLOEXEC | SOCK_NONBLOCK))) {
        s = __socketcall (socket, domain, type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK), protocol, 0, 0, 0);
        if (s < 0)
            return __syscall_ret (s);
        if (type & SOCK_CLOEXEC)
            __syscall (SYS_fcntl, s, F_SETFD, FD_CLOEXEC);
        if (type & SOCK_NONBLOCK)
            __syscall (SYS_fcntl, s, F_SETFL, O_NONBLOCK);
    }

    if (s > -1 && VALID_FD (s)) {
        clear_fd_table_entry (s);
        fd_table[s].domain = domain;
        if (s + 1 > max_fd) {
            max_fd = s + 1;
        }
    }

    s = __syscall_ret (s);
    DEBUG_LOG (" = %d\n", s);
    return s;
}
