
#include "util.h"
#include "desock.h"
#include "test_helper.h"

#ifdef DEBUG

VISIBLE
int _libdesock_fd_table_size (void) {
    return FD_TABLE_SIZE;
}

#endif
