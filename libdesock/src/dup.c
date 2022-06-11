#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

#include "desock.h"
#include "syscall.h"

visible int dup (int fd) {
    int ret = syscall (SYS_dup, fd);

    if (ret > -1 && VALID_FD (ret) && VALID_FD (fd)) {
        DEBUG_LOG ("[%d] desock::dup(%d)", gettid (), fd);

        fd_table[ret].domain = fd_table[fd].domain;
        fd_table[ret].desock = fd_table[fd].desock;
        fd_table[ret].listening = fd_table[fd].listening;
        fd_table[ret].epfd = fd_table[fd].epfd;
        fd_table[ret].ep_event.events = fd_table[fd].ep_event.events;
        fd_table[ret].ep_event.data = fd_table[fd].ep_event.data;

        if (ret + 1 > max_fd) {
            max_fd = ret + 1;
        }

        DEBUG_LOG (" = %d\n", ret);
    }

    return ret;
}

visible int dup2 (int old, int new) {
    int r = new;

    /* Don't allow overwriting of stdin/stdout */
    if (r > 1) {
        while ((r = __syscall (SYS_dup2, old, new)) == -EBUSY) ;
    }

    if (r > -1 && VALID_FD (r) && VALID_FD (old)) {
        DEBUG_LOG ("[%d] desock::dup2(%d, %d)", gettid (), old, new);

        fd_table[r].domain = fd_table[old].domain;
        fd_table[r].desock = fd_table[old].desock;
        fd_table[r].listening = fd_table[old].listening;
        fd_table[r].epfd = fd_table[old].epfd;
        fd_table[r].ep_event.events = fd_table[old].ep_event.events;
        fd_table[r].ep_event.data = fd_table[old].ep_event.data;

        if (r + 1 > max_fd) {
            max_fd = r + 1;
        }

        DEBUG_LOG (" = %d\n", r);
    }

    return __syscall_ret (r);
}

visible int dup3 (int old, int new, int flags) {
    int r = new;

    /* Don't allow overwriting of stdin/stdout */
    if (r > 1) {
        while ((r = __syscall (SYS_dup3, old, new, flags)) == -EBUSY) ;
    }

    if (r > -1 && VALID_FD (r) && VALID_FD (old)) {
        DEBUG_LOG ("[%d] desock::dup3(%d, %d, %d)", gettid (), old, new, flags);

        fd_table[r].domain = fd_table[old].domain;
        fd_table[r].desock = fd_table[old].desock;
        fd_table[r].listening = fd_table[old].listening;
        fd_table[r].epfd = fd_table[old].epfd;
        fd_table[r].ep_event.events = fd_table[old].ep_event.events;
        fd_table[r].ep_event.data = fd_table[old].ep_event.data;

        if (r + 1 > max_fd) {
            max_fd = r + 1;
        }

        DEBUG_LOG (" = %d\n", r);
    }

    return __syscall_ret (r);
}

#ifdef DEBUG

visible int _debug_real_dup2 (int old, int new) {
    int r;
    while ((r = __syscall (SYS_dup2, old, new)) == -EBUSY) ;
    return __syscall_ret (r);
}

#endif
