#include <sys/socket.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

#ifdef DESOCK_CONNECT

VISIBLE
int connect (int fd, const struct sockaddr* addr, socklen_t len) {
    if (UNLIKELY(!has_supported_domain(fd))) {
        return socketcall_cp(connect, fd, addr, len, 0, 0, 0);
    }
    
    DEBUG_LOG("connect(%d, %p, %d)", fd, addr, len);
    fd_table[fd].desock = 1;
    return 0;
}
VERSION(connect)

#endif
