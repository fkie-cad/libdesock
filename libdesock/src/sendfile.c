#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"
#include "musl-features.h"

visible ssize_t sendfile (int out_fd, int in_fd, off_t * ofs, size_t count) {
    if (VALID_FD (out_fd) && fd_table[out_fd].desock) {
        DEBUG_LOG ("[%d] desock::sendfile(%d, %d, %p, %lu) = %lu\n", gettid (), out_fd, in_fd, ofs, count, count);
        return count;
    } else {
        return syscall (SYS_sendfile, out_fd, in_fd, ofs, count);
    }
}

visible strong_alias (sendfile, sendfile64);
