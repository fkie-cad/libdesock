#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "test_helper.h"

#include "tests.h"

int test_epoll (void) {
    int e = epoll_create(0);
    assert(e > 2);
    
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > e);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 1) == 0);
    
    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLOUT,
        .data = {
            .u64 = 0x1337,
        },
    };
    assert(epoll_ctl(e, EPOLL_CTL_ADD, s, &ev) == 0);
    ev = (struct epoll_event) {
        .events = EPOLLOUT,
        .data = {
            .u64 = 1111,
        },
    };
    assert(epoll_ctl(e, EPOLL_CTL_ADD, 1, &ev) == 0);
    
    struct epoll_event results[256] = {0};
    
    /* wait for server ready */
    assert(epoll_wait(e, results, 256, 0) == 1);
    assert(results[0].events == EPOLLIN);
    assert(results[0].data.u64 == 0x1337);
    
    /* get client connection */
    int c = accept(s, NULL, NULL);
    assert(c > s);
    ev = (struct epoll_event) {
        .events = EPOLLIN | EPOLLOUT,
        .data = {
            .u64 = 1234,
        },
    };
    assert(epoll_ctl(e, EPOLL_CTL_ADD, c, &ev) == 0);
    
    /* wait for client ready */
    assert(epoll_wait(e, results, 256, 0) == 1);
    assert(results[0].events == (EPOLLIN | EPOLLOUT));
    assert(results[0].data.u64 == 1234);
    
    /* wait for client again */
    assert(epoll_wait(e, results, 256, 0) == 1);
    assert(results[0].events == (EPOLLIN | EPOLLOUT));
    assert(results[0].data.u64 == 1234);
    
    /* remove client */
    close(c);
    assert(epoll_ctl(e, EPOLL_CTL_DEL, c, NULL) == -1);
    
    /* wait for server ready again */
    assert(epoll_wait(e, results, 256, 0) == 1);
    assert(results[0].events == EPOLLIN);
    assert(results[0].data.u64 == 0x1337);
    
    /* delete server */
    assert(epoll_ctl(e, EPOLL_CTL_DEL, s, NULL) == 0);
    assert(epoll_ctl(e, EPOLL_CTL_DEL, 1, NULL) == 0);
    
    /* wait on empty list */
    assert(epoll_wait(e, results, 256, 0) == 0);
    
    close(s);
    close(e);
    
    return TEST_SUCCESS;
}

int test_passthrough (void) {
    int e = epoll_create(0);
    assert(e > 2);
    
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data = {
            .u64 = 0x1337,
        },
    };
    assert(epoll_ctl(e, EPOLL_CTL_ADD, 0, &ev) == 0);
    
    struct epoll_event results[256] = {0};
    assert(epoll_wait(e, results, 256, 1000) == 0);
    
    return TEST_SUCCESS;
}

test_fn tests [] = {
    test_epoll,
    test_passthrough,
    NULL,
};
