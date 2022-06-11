#define _GNU_SOURCE
#include <sys/select.h>
#include <signal.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"

static int internal_select (int n, fd_set * rfds, fd_set * wfds, fd_set * efds) {
    DEBUG_LOG ("[%d] desock::internal_select(%d, %p, %p, %p)", gettid (), n, rfds, wfds, efds);

    int ret = 0;
    int server_sock = -1;

    accept_block = 0;

    for (int i = 0; i < n; ++i) {
        if (rfds && FD_ISSET (i, rfds)) {
            if (VALID_FD (i) && fd_table[i].desock) {
                if (fd_table[i].listening) {
                    server_sock = i;
                }

                ++ret;
            } else {
                FD_CLR (i, rfds);
            }
        }

        if (wfds && FD_ISSET (i, wfds)) {
            if (VALID_FD (i) && fd_table[i].desock && !fd_table[i].listening) {
                ++ret;
            } else {
                FD_CLR (i, wfds);
            }
        }
    }

    if (efds) {
        explicit_bzero (efds, sizeof (fd_set));
    }

    if (server_sock > -1) {
        if (sem_trywait (&sem) == -1) {
            if (errno != EAGAIN) {
                _error ("desock::internal_select(): sem_trywait failed\n");
            }

            if (ret == 1) {
                sem_wait (&sem);
            } else {
                FD_CLR (server_sock, rfds);
                --ret;
            }
        }
    }

    DEBUG_LOG (" = %d\n", ret);
    return ret;
}

#define IS32BIT(x) !((x)+0x80000000ULL>>32)
#define CLAMP(x) (int)(IS32BIT(x) ? (x) : 0x7fffffffU+((0ULL+(x))>>63))

static int musl_select (int n, fd_set * restrict rfds, fd_set * restrict wfds, fd_set * restrict efds, struct timeval* restrict tv) {
    time_t s = tv ? tv->tv_sec : 0;
    suseconds_t us = tv ? tv->tv_usec : 0;
    long ns;
    const time_t max_time = (1ULL << 8 * sizeof (time_t) - 1) - 1;

    if (s < 0 || us < 0)
        return __syscall_ret (-EINVAL);
    if (us / 1000000 > max_time - s) {
        s = max_time;
        us = 999999;
        ns = 999999999;
    } else {
        s += us / 1000000;
        us %= 1000000;
        ns = us * 1000;
    }

#ifdef SYS_pselect6_time64
    int r = -ENOSYS;
    if (SYS_pselect6 == SYS_pselect6_time64 || !IS32BIT (s))
        r = __syscall_cp (SYS_pselect6_time64, n, rfds, wfds, efds, tv ? ((long long[]) { s, ns }
                          ) : 0,
                          ((syscall_arg_t[]) { 0, _NSIG / 8 }));
    if (SYS_pselect6 == SYS_pselect6_time64 || r != -ENOSYS)
        return __syscall_ret (r);
#endif
#ifdef SYS_select
    return syscall_cp (SYS_select, n, rfds, wfds, efds, tv ? ((long[]) { s, us }) : 0);
#else
    return syscall_cp (SYS_pselect6, n, rfds, wfds, efds, tv ? ((long[]) { s, ns }) : 0, ((syscall_arg_t[]) { 0, _NSIG / 8 }));
#endif
}

visible int select (int n, fd_set * restrict rfds, fd_set * restrict wfds, fd_set * restrict efds, struct timeval* restrict tv) {
    if (!rfds && !wfds && !efds) {
        return musl_select (n, rfds, wfds, efds, tv);
    } else {
        return internal_select (n, rfds, wfds, efds);
    }

}

visible int pselect (int n, fd_set * restrict rfds, fd_set * restrict wfds, fd_set * restrict efds, const struct timespec* restrict ts, const sigset_t * restrict mask) {
    if (!rfds && !wfds && !efds) {
        return musl_select (n, rfds, wfds, efds, ts);
    } else {
        return internal_select (n, rfds, wfds, efds);
    }
}
