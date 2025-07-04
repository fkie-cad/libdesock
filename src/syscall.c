#include "syscall.h"

static long sccp (syscall_arg_t nr, syscall_arg_t u, syscall_arg_t v, syscall_arg_t w, syscall_arg_t x, syscall_arg_t y, syscall_arg_t z) {
    return __syscall (nr, u, v, w, x, y, z);
}

weak_alias (sccp, __syscall_cp_c);

long (__syscall_cp) (syscall_arg_t nr, syscall_arg_t u, syscall_arg_t v, syscall_arg_t w, syscall_arg_t x, syscall_arg_t y, syscall_arg_t z) {
    return __syscall_cp_c (nr, u, v, w, x, y, z);
}

long __syscall_ret (unsigned long r) {
    if (r > -4096UL) {
        errno = -r;
        return -1;
    } else {
        return r;
    }
}
