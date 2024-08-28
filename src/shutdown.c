#include <sys/socket.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

VISIBLE
int shutdown (int fd, int how) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall(shutdown, fd, how, 0, 0, 0, 0);
    }
    
    DEBUG_LOG("shutdown(%d, %d)", fd, how);
    return 0;
}
VERSION(shutdown)
