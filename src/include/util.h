#ifndef _LIBDESOCK_UTIL_H
#define _LIBDESOCK_UTIL_H

#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define LIKELY(x) __builtin_expect(!!(x), 1)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#ifdef SHARED
#define VISIBLE __attribute__((visibility ("default")))
#else
#define VISIBLE
#endif

#endif /* _LIBDESOCK_UTIL_H */
