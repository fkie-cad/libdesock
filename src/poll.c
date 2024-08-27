#include <poll.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

static int do_poll (struct pollfd* fds, nfds_t n) {
    int r = 0;
    int server_sock = -1;
    
    accept_block = 0;
    
    for (nfds_t i = 0; i < n; ++i) {
        if (LIKELY(DESOCK_FD(fds[i].fd))) {
            fds[i].revents = fds[i].events & (POLLIN | POLLOUT);

            if (fd_table[fds[i].fd].listening) {
                server_sock = i;
                fds[i].revents &= (~POLLOUT);
            }

            ++r;
        } else {
            fds[i].revents = 0;
        }
    }
    
    if (server_sock > -1) {
        if (UNLIKELY(sem_trywait(&sem) == -1)) {
            if (UNLIKELY(errno != EAGAIN)) {
                _error("poll(): sem_trywait failed");
            }

            if (r <= 1) {
                sem_wait(&sem);
            } else {
                fds[server_sock].revents = 0;
                --r;
            }
        }
    }
    
    return r;
}

VISIBLE
int poll (struct pollfd* fds, nfds_t n, int timeout) {
    DEBUG_LOG("poll(%p, %d, %d)", fds, n, timeout);
    
    int r = do_poll(fds, n);

    if (UNLIKELY(r == 0)) {
        return syscall_cp(SYS_poll, fds, n, timeout);
    }
    
    return r;
}
VERSION(poll)

VISIBLE
int ppoll (struct pollfd* fds, nfds_t n, const struct timespec* to, const sigset_t* mask) {
    DEBUG_LOG("ppoll(%p, %d, %p, %p)", fds, n, to, mask);
    
    int r = do_poll(fds, n);

    if (UNLIKELY(r == 0)) {
        return syscall_cp(SYS_ppoll, fds, n, to, mask);
    }
    
    return r;
}
VERSION(ppoll)
