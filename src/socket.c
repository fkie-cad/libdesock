#include <sys/socket.h>
#include <fcntl.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

VISIBLE
int socket (int domain, int type, int protocol) {
    DEBUG_LOG("socket(%d, %d, %d)", domain, type, protocol);

    int s = __socketcall(socket, domain, type, protocol, 0, 0, 0);
    if ((s == -EINVAL || s == -EPROTONOSUPPORT)
        && (type & (SOCK_CLOEXEC | SOCK_NONBLOCK))) {
        s = __socketcall(socket, domain, type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK), protocol, 0, 0, 0);
        if (s < 0)
            return __syscall_ret(s);
        if (type & SOCK_CLOEXEC)
            __syscall(SYS_fcntl, s, F_SETFD, FD_CLOEXEC);
        if (type & SOCK_NONBLOCK)
            __syscall(SYS_fcntl, s, F_SETFL, O_NONBLOCK);
    }

    if (LIKELY(VALID_FD(s))) {
        clear_fd_table_entry(s);
        fd_table[s].domain = domain;
        
        if (LIKELY(s + 1 > max_fd)) {
            max_fd = s + 1;
        }
    }

    s = __syscall_ret(s);
    DEBUG_LOG("    => %d", s);
    return s;
}
VERSION(socket)
