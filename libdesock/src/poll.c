#define _GNU_SOURCE
#include <poll.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"

static int internal_poll (struct pollfd* fds, nfds_t n) {
    DEBUG_LOG ("[%d] desock::internal_poll(%p, %d)", gettid (), fds, n);

    int ret = 0;
    int server_sock = -1;

    accept_block = 0;

    for (nfds_t i = 0; i < n; ++i) {
        if (VALID_FD (fds[i].fd) && fd_table[fds[i].fd].desock) {
            fds[i].revents = fds[i].events & (POLLIN | POLLOUT);

            if (fd_table[fds[i].fd].listening) {
                server_sock = i;
                fds[i].revents &= (~POLLOUT);
            }

            ++ret;
        } else {
            fds[i].revents = 0;
        }
    }

    if (server_sock > -1) {
        if (sem_trywait (&sem) == -1) {
            if (errno != EAGAIN) {
                _error ("desock::internal_poll(): sem_trywait failed\n");
            }

            if (ret == 1) {
                sem_wait (&sem);
            } else {
                fds[server_sock].revents = 0;
                --ret;
            }
        }
    }

    DEBUG_LOG (" = %d\n", ret);
    return ret;
}

visible int poll (struct pollfd* fds, nfds_t n, int timeout) {
    int r = internal_poll (fds, n);

    if (r == 0) {
        return syscall_cp (SYS_poll, fds, n, timeout);
    } else {
        return r;
    }
}

visible int ppoll (struct pollfd* fds, nfds_t n, const struct timespec* to, const sigset_t * mask) {
    int r = internal_poll (fds, n);

    if (r == 0) {
        return syscall_cp (SYS_ppoll, fds, n, to, mask);
    } else {
        return r;
    }
}
