/* The default action for libdesock is to
 * read from stdin / write to stdout
 * but this behaviour can be changed with the
 * following to functions.
 * They have to behave like glibc functions:
 * On success they must return a value >= 0 indicating
 * how many bytes have been read / written.
 * On failure they must return -1 with errno set to the
 * corresponding error.
 */
#include "hooks.h"
#include "syscall.h"
 

/* This function is called whenever a read on a network
 * connection occurs. Read from stdin instead.
 */
ssize_t hook_input (char* buf, size_t size) {
    return syscall_cp(SYS_read, 0, buf, size);
}

/* This function is called whenever a write on a network
 * connection occurs. Write to stdout instead.
 */
ssize_t hook_output (char* buf, size_t size) {
#ifdef DEBUG
    return syscall_cp(SYS_write, 1, buf, size);
#else
    return (ssize_t) size;
#endif
}
