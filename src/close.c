#include "util.h"
#include "desock.h"
#include "syscall.h"

VISIBLE
int close (int fd) {
    int r = 0;
    int sem_value = 0;

    if (LIKELY(VALID_FD(fd))) {
        if (fd_table[fd].desock) {
            DEBUG_LOG("close(%d)", fd);
        }

        if (fd_table[fd].desock && !fd_table[fd].listening) {
            if (UNLIKELY(sem_getvalue(&sem, &sem_value) == -1)) {
                return -1;
            }

            if (sem_value < MAX_CONNS) {
                sem_post(&sem);
            }
        }

        clear_fd_table_entry(fd);
    }

    /* Allow close() for every fd except stdin + stdout + stderr */
    if (fd > 2) {
        r = __syscall_cp(SYS_close, fd);
        if (r == -EINTR)
            r = 0;
    }

    return __syscall_ret(r);
}
VERSION(close)
