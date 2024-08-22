#include <sys/socket.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"
#include "stub_sockaddr.h"

VISIBLE
int getsockname (int fd, struct sockaddr* addr, socklen_t* len) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        return socketcall(getsockname, fd, addr, len, 0, 0, 0);
    }
    
    DEBUG_LOG("getsockname(%d, %p, %p)", fd, addr, len);
    fill_sockaddr(fd, addr, len);
    return 0;
}
VERSION(getsockname)
