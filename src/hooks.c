/*  Hooks let you customize what happens
 *  when an application reads from / writes to
 *  a network connection.
 *  The default behavior is to read from stdin /
 *  write to stdout but this can easily be changed
 *  in the functions below.
 */

#include <unistd.h>

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
ssize_t hook_output (const char* buf, size_t size) {
#ifdef DEBUG
    return syscall_cp(SYS_write, 1, buf, size);
#else
    (void) buf;
    return (ssize_t) size;
#endif
}

/*  This function is called whenever libdesock internally
 *  searches through the input stream. It MUST behave like
 *  the lseek() function in the sense that on success, it
 *  must return the resulting offset and on error it 
 *  must return -1.
 *  The supplied offset always is relative to the current
 *  stream position.
 */
off_t hook_seek (off_t offset) {
    return lseek(0, offset, SEEK_CUR);
}
