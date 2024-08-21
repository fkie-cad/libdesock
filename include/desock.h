#ifndef _LIBDESOCK_DESOCK_H
#define _LIBDESOCK_DESOCK_H

#include <semaphore.h>
#include <sys/epoll.h>

/*
 * If conns >= MAX_CONNS, accept() will block
 */
#ifndef MAX_CONNS
#define MAX_CONNS 1
#endif

#ifndef FD_TABLE_SIZE
#define FD_TABLE_SIZE 4096
#endif

#ifdef DEBUG
void _debug (char*, ...);
#define DEBUG_LOG(...) _debug(__VA_ARGS__);
#else
#define DEBUG_LOG(...)
#endif

#define VALID_FD(x) (0 <= (x) && (x) < FD_TABLE_SIZE)

struct fd_entry {
    /* information passed to socket() */
    int domain;
    
    /* flag whether to desock this fd */
    int desock;
    
    /* flag whether this is the server socket */
    int listening;
    
    /* epoll stuff */
    int epfd;
    struct epoll_event ep_event;
};

void _error (const char*, ...);

extern sem_t sem;
extern struct fd_entry fd_table[FD_TABLE_SIZE];
extern int accept_block;
extern int max_fd;

#endif /* _LIBDESOCK_DESOCK_H */