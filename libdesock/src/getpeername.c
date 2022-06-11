#define _GNU_SOURCE
#define __USE_GNU
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"

visible int getpeername (int fd, struct sockaddr* restrict addr, socklen_t * restrict len) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        DEBUG_LOG ("[%d] desock::getpeername(%d, %p, %p) = 0\n", gettid (), fd, addr, len);
        fill_sockaddr (fd, addr, len);
        return 0;
    } else {
        return socketcall (getpeername, fd, addr, len, 0, 0, 0);
    }
}
