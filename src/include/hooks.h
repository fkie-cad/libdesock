#ifndef _LIBDESOCK_HOOKS_H
#define _LIBDESOCK_HOOKS_H

#include <stdlib.h>

ssize_t hook_input (char*, size_t);
ssize_t hook_output (char*, size_t);
ssize_t hook_seek (off_t);

#endif /* _LIBDESOCK_HOOKS_H */
