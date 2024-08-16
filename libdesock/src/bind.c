#ifdef DESOCK_BIND
#include <sys/socket.h>

#define __USE_GNU
#define _GNU_SOURCE
#include <unistd.h>

#include "syscall.h"
#include "desock.h"

visible int bind (int fd, const struct sockaddr* addr, socklen_t len) {
    if (VALID_FD (fd) && DESOCK_FD (fd)) {
        DEBUG_LOG ("[%d] desock::bind(%d, %p, %d) = 0\n", gettid (), fd, addr, len);
        fd_table[fd].desock = 1;
        return 0;
    } else {
        return socketcall (bind, fd, addr, len, 0, 0, 0);
    }
}

#ifdef DEBUG
visible int _debug_real_bind (int fd, const struct sockaddr* addr, socklen_t len) {
    return socketcall (bind, fd, addr, len, 0, 0, 0);
}
#endif

#endif
