#define _GNU_SOURCE
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"

visible int setsockopt (int fd, int level, int optname, const void* optval, socklen_t optlen) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        DEBUG_LOG ("[%d] desock::setsockopt(%d, %d, %d, %p, %lu) = 0\n", gettid (), fd, level, optname, optval, optlen);
        return 0;
    } else {
        int r = __socketcall (setsockopt, fd, level, optname, optval, optlen, 0);
        return __syscall_ret (r);
    }
}
