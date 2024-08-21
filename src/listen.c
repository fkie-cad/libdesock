#include "util.h"
#include "desock.h"
#include "syscall.h"

VISIBLE
int listen (int fd, int backlog) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall (listen, fd, backlog, 0, 0, 0, 0);
    }
    
    DEBUG_LOG("listen(%d, %d)", fd, backlog);
    fd_table[fd].listening = 1;
    return 0;
}
