#include <unistd.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"
#include "musl-features.h"
#include "hooks.h"

VISIBLE
ssize_t sendfile (int out_fd, int in_fd, off_t* ofs, size_t count) {
    if (UNLIKELY(!DESOCK_FD(out_fd))) {
        return syscall(SYS_sendfile, out_fd, in_fd, ofs, count);
    }
    
    DEBUG_LOG("sendfile(%d, %d, %p, %lu) = %lu", out_fd, in_fd, ofs, count, count);

#ifdef DEBUG
    off_t old_offset = 0;
    size_t consumed = 0;
    char buf[512];

    if (ofs) {
        old_offset = lseek(in_fd, 0, SEEK_CUR);

        if (old_offset < 0) {
            _error("Cannot lseek() in sendfile() in_fd");
        }

        if (lseek(in_fd, *ofs, SEEK_SET) < 0) {
            _error("Invalid ofs in sendfile()");
        }
    }

    while (consumed < count) {
        size_t delta = count - consumed;
        ssize_t r = syscall_cp(SYS_read, in_fd, buf, MIN(sizeof(buf), delta));

        if (r < 0) {
            _error("Cannot read from file in sendfile()");
        }

        hook_output(buf, (size_t) r);

        consumed += (size_t) r;
    }

    if (ofs) {
        if (lseek(in_fd, old_offset, SEEK_SET) < 0) {
            _error("Cannot restore file offset in sendfile()");
        }
    }
#endif
    
    return count;
}
VERSION(sendfile)

VISIBLE strong_alias(sendfile, sendfile64);
VERSION(sendfile64)
