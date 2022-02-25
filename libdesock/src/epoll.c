#define _GNU_SOURCE
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#include <desock.h>
#include <syscall.h>

#ifdef DEBUG
int epoll_create (int size) {
    int r = __syscall_ret (__syscall (SYS_epoll_create1, 0));
    DEBUG_LOG ("[%d] desock::epoll_create(%d) = %d\n", gettid (), size, r);
    return r;
}

int epoll_create1 (int flags) {
    int r = __syscall_ret (__syscall (SYS_epoll_create1, flags));
    DEBUG_LOG ("[%d] desock::epoll_create1(%d) = %d\n", gettid (), flags, r);
    return r;
}
#endif

int epoll_ctl (int fd, int op, int fd2, struct epoll_event* ev) {
    if (VALID_FD (fd2)) {
        DEBUG_LOG ("[%d] desock::epoll_ctl(%d, %d, %d, %p)", gettid (), fd, op, fd2, ev);

        if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
            fd_table[fd2].epfd = fd;
            fd_table[fd2].ep_event.events = ev->events;
            fd_table[fd2].ep_event.data = ev->data;
        } else if (op == EPOLL_CTL_DEL) {
            fd_table[fd2].epfd = -1;
        }

        if (fd_table[fd2].desock) {
            DEBUG_LOG (" = 0\n");
            return 0;
        }
    }

    int r = syscall (SYS_epoll_ctl, fd, op, fd2, ev);
#ifdef DEBUG
    if (VALID_FD (fd2)) {
        DEBUG_LOG (" = %d\n", r);
    }
#endif
    return r;
}

static int internal_epoll_wait (int fd, struct epoll_event* ev, int cnt) {
    int j = 0;
    int server_sock = -1;

    accept_block = 0;

    for (int i = 0; i < max_fd && j < cnt; ++i) {
        if (fd_table[i].desock && fd_table[i].epfd == fd) {
            if (fd_table[i].listening) {
                server_sock = i;
            } else {
                ev[j].events = fd_table[i].ep_event.events;
                ev[j].events &= (EPOLLIN | EPOLLOUT);
                ev[j].data = fd_table[i].ep_event.data;
                ++j;

                if (fd_table[i].ep_event.events & EPOLLONESHOT) {
                    fd_table[i].epfd = -1;
                }
            }
        }
    }

    if (server_sock > -1 && j < cnt) {
        if (sem_trywait (&sem) == -1) {
            if (errno != EAGAIN) {
                _error ("desock::internal_epoll_wait(): sem_trywait failed\n");
            }

            if (j > 0) {
                return j;
            }

            sem_wait (&sem);
        }

        ev[j].events = fd_table[server_sock].ep_event.events;
        ev[j].events &= EPOLLIN;
        ev[j].data = fd_table[server_sock].ep_event.data;
        ++j;

        if (fd_table[server_sock].ep_event.events & EPOLLONESHOT) {
            fd_table[server_sock].epfd = -1;
        }
    }

    return j;
}

int epoll_pwait (int fd, struct epoll_event* ev, int cnt, int to, const sigset_t * sigs) {
    DEBUG_LOG ("[%d] desock::epoll_pwait(%d, %p, %d, %d, %p)", gettid (), fd, ev, cnt, to, sigs);

    int ret = internal_epoll_wait (fd, ev, cnt);
    if (ret) {
        DEBUG_LOG (" = %d\n", ret);
        return ret;
    } else {
        ret = __syscall_ret (__syscall (SYS_epoll_pwait, fd, ev, cnt, to, sigs));
        DEBUG_LOG (" = %d\n", ret);
        return ret;
    }
}

int epoll_wait (int fd, struct epoll_event* ev, int cnt, int to) {
    DEBUG_LOG ("[%d] desock::epoll_wait(%d, %p, %d, %d)", gettid (), fd, ev, cnt, to);

    int ret = internal_epoll_wait (fd, ev, cnt);
    if (ret) {
        DEBUG_LOG (" = %d\n", ret);
        return ret;
    } else {
        ret = __syscall_ret (__syscall (SYS_epoll_pwait, fd, ev, cnt, to, 0));
        DEBUG_LOG (" = %d\n", ret);
        return ret;
    }
}

int epoll_pwait2 (int epfd, struct epoll_event* events, int maxevents, struct timespec* timeout, const sigset_t * sigmask) {
    DEBUG_LOG ("[%d] desock::epoll_pwait2(%d, %p, %d, %p, %p)", gettid (), epfd, events, maxevents, timeout, sigmask);

    int ret = internal_epoll_wait (epfd, events, maxevents);
    if (ret) {
        DEBUG_LOG (" = %d\n", ret);
        return ret;
    } else {
        errno = ENOSYS;
        DEBUG_LOG (" = -1\n");
        return -1;
    }
}
