#define _GNU_SOURCE
#define __USE_GNU
#include <sys/socket.h>
#include <unistd.h>

#include <desock.h>
#include <syscall.h>

visible int shutdown (int fd, int how) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        DEBUG_LOG ("[%d] desock::shutdown(%d, %d) = 0\n", gettid (), fd, how);
        return 0;
    } else {
        return socketcall (shutdown, fd, how, 0, 0, 0, 0);
    }
}
