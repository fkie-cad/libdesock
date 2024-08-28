
#include "util.h"
#include "desock.h"
#include "syscall.h"
#include "musl-features.h"

VISIBLE
ssize_t sendfile (int out_fd, int in_fd, off_t* ofs, size_t count) {
    if (UNLIKELY(!DESOCK_FD(out_fd))) {
        return syscall(SYS_sendfile, out_fd, in_fd, ofs, count);
    }
    
    DEBUG_LOG("sendfile(%d, %d, %p, %lu) = %lu", out_fd, in_fd, ofs, count, count);
    return count;
}
VERSION(sendfile)

VISIBLE strong_alias(sendfile, sendfile64);
VERSION(sendfile64)
