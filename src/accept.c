#include <sys/socket.h>

#include "util.h"
#include "syscall.h"
#include "desock.h"
#include "stub_sockaddr.h"

VISIBLE
int accept4 (int fd, struct sockaddr* addr, socklen_t* len, int flag) {
    if (UNLIKELY(!VALID_FD(fd) || !fd_table[fd].desock)) {
        return socketcall_cp(accept4, fd, addr, len, flag, 0, 0);
    }
    
    DEBUG_LOG("accept4(%d, %p, %p, %d)", fd, addr, len, flag);

    if (LIKELY(accept_block)) {
        sem_wait(&sem);
    }
    accept_block = 1;

    int new_fd = syscall(SYS_dup, fd);

    if (UNLIKELY(!VALID_FD(new_fd))) {
        DEBUG_LOG("    => -1");
        return -1;
    }

    clone_fd_table_entry(new_fd, fd);
    fd_table[new_fd].listening = 0;

    fill_sockaddr(fd, addr, len);

    if (LIKELY(new_fd + 1 > max_fd)) {
        max_fd = new_fd + 1;
    }

    DEBUG_LOG("    => %d\n", new_fd);
    return new_fd;
}

VISIBLE
int accept(int fd, struct sockaddr* addr, socklen_t* len) {
    return accept4(fd, addr, len, 0);
}
