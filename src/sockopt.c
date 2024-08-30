#include "util.h"
#include "desock.h"
#include "syscall.h"

VISIBLE
int setsockopt (int fd, int level, int optname, const void* optval, socklen_t optlen) {
    if (UNLIKELY(!DESOCK_FD(fd))) {
        int r = __socketcall(setsockopt, fd, level, optname, optval, optlen, 0);
        return __syscall_ret(r);
    }
    
    DEBUG_LOG("setsockopt(%d, %d, %d, %p, %lu)", fd, level, optname, optval, optlen);
    return 0;
}
VERSION(setsockopt)
