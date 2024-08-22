#include <sys/socket.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

#ifdef DESOCK_BIND

VISIBLE
int bind (int fd, const struct sockaddr* addr, socklen_t len) {
    if (UNLIKELY(!has_supported_domain(fd))) {
        return socketcall(bind, fd, addr, len, 0, 0, 0);
    }
    
    DEBUG_LOG("bind(%d, %p, %d)", fd, addr, len);
    fd_table[fd].desock = 1;
    return 0;
}
VERSION(bind)

#endif
