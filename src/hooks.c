/*  Hooks let you customize what happens
 *  when an application reads from / writes to
 *  a network connection.
 *  The default behavior is to read from stdin /
 *  write to stdout but this can easily be changed
 *  in the functions below.
 */

#include "hooks.h"
#include "syscall.h"
 
/*  This function is called whenever a read on a network
 *  connection occurs. It MUST return the number of bytes
 *  written to buf or -1 if an error occurs.
 */
ssize_t hook_input (char* buf, size_t size) {
    return syscall_cp(SYS_read, 0, buf, size);
}

/*  This function is called whenever a write on a network
 *  connection occurs. It MUST return the number of bytes
 *  written or -1 if an error occurs.
 */
ssize_t hook_output (char* buf, size_t size) {
#ifdef DEBUG
    return syscall_cp(SYS_write, 1, buf, size);
#else
    return (ssize_t) size;
#endif
}
