#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>

#include "util.h"
#include "desock.h"

// Check that we are desocketing at least client or server
#ifndef DESOCK_BIND
#ifndef DESOCK_CONNECT
#error "At least one of desock_client or desock_server must be specified"
#endif
#endif

#ifndef MAX_CONNS
#error "max_conns was not set"
#endif

#ifndef FD_TABLE_SIZE
#error "fd_table_size was not set"
#endif

/* Highest file descriptor number seen so far */
int max_fd = 0;

/* Indicates whether the next call to accept() should block or not */
int accept_block = 1;

/* Table that holds metadata about desocketed file descriptors */
struct fd_entry fd_table[FD_TABLE_SIZE];

/* Semaphore for synchronization of the connection pool in multi-threaded applications. */
sem_t sem;


void _debug (const char* fmt_string, ...) {
    va_list args;
    va_start(args, fmt_string);
    
    fprintf(stderr, "[libdesock::debug] <%d> ", gettid());
    vfprintf(stderr, fmt_string, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    fflush(stderr);
}

__attribute__((noreturn))
void _error (const char* fmt_string, ...) {
    va_list args;
    va_start(args, fmt_string);
    
    fprintf(stderr, "[libdesock::error] ");
    vfprintf(stderr, fmt_string, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    fflush(stderr);
    
    while (1) {
        abort();
    }
}

__attribute__((constructor))
void __libdesock_init (void) {
    if (sem_init(&sem, 1, MAX_CONNS) == -1) {
        _error("sem_init() failed: %s", strerror(errno));
    }
    
    DEBUG_LOG("libdesock loaded");
}
