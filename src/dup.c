/*
 * Added optional support for duplicating stdin.
 *     By: Kelly Patterson - Cisco Talos
 *     Copyright (C) 2024 Cisco Systems Inc
 */
 
 #include <unistd.h>
 
 #include "util.h"
 #include "desock.h"
 #include "syscall.h"
 
 VISIBLE
 int dup (int fd) {
    int ret = syscall(SYS_dup, fd);
    
    if (LIKELY(VALID_FD(ret) && VALID_FD(fd))) {
        DEBUG_LOG("dup(%d) = %d", fd, ret);

        clone_fd_table_entry(ret, fd);

        if (LIKELY(ret + 1 > max_fd)) {
            max_fd = ret + 1;
        }
    }

    return ret;
}
VERSION(dup)

VISIBLE
int dup2 (int old, int new) {
    int r = new;

    /* Don't allow overwriting of stdin/stdout */
    if (LIKELY(r > 1)) {
        while ((r = __syscall(SYS_dup2, old, new)) == -EBUSY) ;
    }
#ifdef DUP_STDIN
    /* except in this case do allow overwriting of stdin*/
    else if (r==0) {
        while ((r = __syscall(SYS_dup2, old, new)) == -EBUSY) ;
    }
#endif

    if (LIKELY(VALID_FD(r) && VALID_FD(old))) {
        DEBUG_LOG("dup2(%d, %d) = %d", old, new, r);

        clone_fd_table_entry(r, old);

        if (LIKELY(r + 1 > max_fd)) {
            max_fd = r + 1;
        }
    }

    return __syscall_ret(r);
}
VERSION(dup2)

VISIBLE
int dup3 (int old, int new, int flags) {
    int r = new;

    /* Don't allow overwriting of stdin/stdout */
    if (r > 1) {
        while ((r = __syscall(SYS_dup3, old, new, flags)) == -EBUSY) ;
    }
#ifdef DUP_STDIN
    /* except in this case do allow overwriting of stdin*/
    else if (r==0) {
        while ((r = __syscall(SYS_dup3, old, new, flags)) == -EBUSY) ;
    }
#endif

    if (LIKELY(VALID_FD(r) && VALID_FD(old))) {
        DEBUG_LOG("dup3(%d, %d, %d) = %d", old, new, flags, r);

        clone_fd_table_entry(r, old);

        if (LIKELY(r + 1 > max_fd)) {
            max_fd = r + 1;
        }
    }

    return __syscall_ret(r);
}
VERSION(dup3)
