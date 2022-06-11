#ifdef DESOCK_CONNECT
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/socket.h>

#include <syscall.h>
#include <desock.h>

visible int connect (int fd, const struct sockaddr* addr, socklen_t len) {
    if (VALID_FD (fd) && DESOCK_FD (fd)) {
        DEBUG_LOG ("[%d] desock::connect(%d, %p, %d)\n", gettid (), fd, addr, len);
        fd_table[fd].desock = 1;
        return 0;
    } else {
        return socketcall_cp (connect, fd, addr, len, 0, 0, 0);
    }
}
#endif
