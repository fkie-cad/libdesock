#include <sys/epoll.h>
#include <errno.h>

#include "util.h"
#include "desock.h"
#include "syscall.h"

#ifdef DEBUG

VISIBLE
int epoll_create (int size) {
    int r = __syscall_ret(__syscall(SYS_epoll_create1, 0));
    DEBUG_LOG("epoll_create(%d) = %d", size, r);
    return r;
}
VERSION(epoll_create)

VISIBLE
int epoll_create1 (int flags) {
    int r = __syscall_ret(__syscall(SYS_epoll_create1, flags));
    DEBUG_LOG("epoll_create1(%d) = %d", flags, r);
    return r;
}
VERSION(epoll_create1)

#endif

VISIBLE
int epoll_ctl (int fd, int op, int fd2, struct epoll_event* ev) {
    if (LIKELY(VALID_FD(fd2))) {
        DEBUG_LOG("epoll_ctl(%d, %d, %d, %p)", fd, op, fd2, ev);

        if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
            fd_table[fd2].epfd = fd;
            __builtin_memcpy(&fd_table[fd2].ep_event, ev, sizeof(struct epoll_event));
        } else if (op == EPOLL_CTL_DEL) {
            fd_table[fd2].epfd = -1;
        }

        if (LIKELY(fd_table[fd2].desock)) {
            return 0;
        }
    }

    return syscall(SYS_epoll_ctl, fd, op, fd2, ev);
}
VERSION(epoll_ctl)

static int do_wait (int fd, struct epoll_event* ev, int cnt) {
    int j = 0;
    int server_sock = -1;

    accept_block = 0;

    for (int i = 0; i < max_fd && j < cnt; ++i) {
        if (fd_table[i].desock && fd_table[i].epfd == fd) {
            if (fd_table[i].listening) {
                server_sock = i;
            } else {
                ev[j].events = fd_table[i].ep_event.events & (EPOLLIN | EPOLLOUT);
                ev[j].data = fd_table[i].ep_event.data;
                ++j;

                if (UNLIKELY(fd_table[i].ep_event.events & EPOLLONESHOT)) {
                    fd_table[i].epfd = -1;
                }
            }
        }
    }

    if (server_sock > -1 && j < cnt) {
        if (UNLIKELY(sem_trywait(&sem) == -1)) {
            if (errno != EAGAIN) {
                _error("Cannot wait for semaphore in epoll implementation");
            }

            if (j > 0) {
                return j;
            }

            sem_wait(&sem);
        }

        ev[j].events = fd_table[server_sock].ep_event.events & EPOLLIN;
        ev[j].data = fd_table[server_sock].ep_event.data;
        ++j;

        if (UNLIKELY(fd_table[server_sock].ep_event.events & EPOLLONESHOT)) {
            fd_table[server_sock].epfd = -1;
        }
    }

    return j;
}

VISIBLE
int epoll_pwait (int fd, struct epoll_event* ev, int cnt, int to, const sigset_t * sigs) {
    DEBUG_LOG("epoll_pwait(%d, %p, %d, %d, %p)", fd, ev, cnt, to, sigs);

    int ret = do_wait(fd, ev, cnt);
    
    if (LIKELY(ret)) {
        return ret;
    } 
    
    return __syscall_ret(__syscall(SYS_epoll_pwait, fd, ev, cnt, to, sigs));
}
VERSION(epoll_pwait)

VISIBLE
int epoll_wait (int fd, struct epoll_event* ev, int cnt, int to) {
    DEBUG_LOG("epoll_wait(%d, %p, %d, %d)", fd, ev, cnt, to);

    int ret = do_wait(fd, ev, cnt);
    
    if (LIKELY(ret)) {
        return ret;
    }
    
    return __syscall_ret(__syscall(SYS_epoll_pwait, fd, ev, cnt, to, 0));
}
VERSION(epoll_wait)

VISIBLE
int epoll_pwait2 (int epfd, struct epoll_event* events, int maxevents, const struct timespec* timeout, const sigset_t * sigmask) {
    DEBUG_LOG("epoll_pwait2(%d, %p, %d, %p, %p)", epfd, events, maxevents, timeout, sigmask);

    int ret = do_wait(epfd, events, maxevents);
    
    if (LIKELY(ret)) {
        return ret;
    } 
    
    errno = ENOSYS;
    return -1;
}
VERSION(epoll_pwait2)
