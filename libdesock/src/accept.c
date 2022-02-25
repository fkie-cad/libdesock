#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <netinet/in.h>

#include <syscall.h>
#include <desock.h>

static int internal_accept (int fd, struct sockaddr* restrict addr, socklen_t * restrict len, int flag) {
    if (VALID_FD (fd) && fd_table[fd].desock) {
        DEBUG_LOG ("[%d] desock::internal_accept(%d, %p, %p, %d)", gettid (), fd, addr, len, flag);

        if (accept_block) {
            sem_wait (&sem);
        } else {
            accept_block = 1;
        }

        int new_fd = syscall (SYS_dup, fd);

        if (new_fd == -1 || !VALID_FD (new_fd)) {
            DEBUG_LOG (" = -1\n");
            return -1;
        }

        clear_fd_table_entry (new_fd);
        fd_table[new_fd].domain = fd_table[fd].domain;
        fd_table[new_fd].desock = 1;

        fill_sockaddr (fd, addr, len);

        if (new_fd + 1 > max_fd) {
            max_fd = new_fd + 1;
        }

        DEBUG_LOG (" = %d\n", new_fd);
        return new_fd;
    } else {
        return socketcall_cp (accept4, fd, addr, len, flag, 0, 0);
    }
}

int accept (int fd, struct sockaddr* restrict addr, socklen_t * restrict len) {
    return internal_accept (fd, addr, len, 0);
}

int accept4 (int fd, struct sockaddr* restrict addr, socklen_t * restrict len, int flg) {
    return internal_accept (fd, addr, len, flg);
}
