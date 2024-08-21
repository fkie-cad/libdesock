#ifndef _LIBDESOCK_DESOCK_H
#define _LIBDESOCK_DESOCK_H

#include <sys/socket.h>
#include <semaphore.h>
#include <sys/epoll.h>

/* If conns >= MAX_CONNS, accept() will block */
#ifndef MAX_CONNS
#define MAX_CONNS 1
#endif

#ifndef FD_TABLE_SIZE
#define FD_TABLE_SIZE 4096
#endif

#ifdef DEBUG
void _debug (const char*, ...);
#define DEBUG_LOG(...) _debug(__VA_ARGS__);
#else
#define DEBUG_LOG(...)
#endif

#define VALID_FD(x) (0 <= (x) && (x) < FD_TABLE_SIZE)
#define DESOCK_FD(x) (VALID_FD(x) && fd_table[(x)].desock)

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

__attribute__((always_inline))
static inline void clear_fd_table_entry (int fd) {
    fd_table[fd].domain = -1;
    fd_table[fd].desock = 0;
    fd_table[fd].listening = 0;
    fd_table[fd].epfd = -1;
}

__attribute__((always_inline))
static inline void clone_fd_table_entry (int to, int from) {
    __builtin_memcpy(&fd_table[to], &fd_table[from], sizeof(struct fd_entry));
}

__attribute__((always_inline))
static inline int has_supported_domain (int fd) {
    if (UNLIKELY(!VALID_FD(fd))) {
        return 0;
    }
    int domain = fd_table[fd].domain;
    return (domain == AF_INET || domain == AF_INET6);
}

#endif /* _LIBDESOCK_DESOCK_H */
