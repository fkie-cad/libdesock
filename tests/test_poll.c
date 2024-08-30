#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>

#include "test_helper.h"

#include "tests.h"

int test_poll (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 1) == 0);
    
    struct pollfd fds[2] = {0};
    
    fds[0] = (struct pollfd) {
        .fd = s,
        .events = POLLIN | POLLOUT,
        .revents = 0,
    };
    fds[1] = (struct pollfd) {
        .fd = 1,
        .events = POLLOUT,
        .revents = 0,
    };
    
    assert(poll(fds, 2, 0) == 1);
    assert(fds[0].fd == s);
    assert(fds[0].events == (POLLIN | POLLOUT));
    assert(fds[0].revents == POLLIN);
    assert(fds[1].fd == 1);
    assert(fds[1].events == POLLOUT);
    assert(fds[1].revents == 0);
    
    int c = accept(s, NULL, NULL);
    assert(c > s);
    
    fds[1] = (struct pollfd) {
        .fd = c,
        .events = POLLIN | POLLOUT,
        .revents = 0,
    };
    
    assert(poll(fds, 2, 0) == 1);
    assert(fds[0].fd == s);
    assert(fds[0].events == (POLLIN | POLLOUT));
    assert(fds[0].revents == 0);
    assert(fds[1].fd == c);
    assert(fds[1].events == (POLLIN | POLLOUT));
    assert(fds[1].revents == (POLLIN | POLLOUT));
    
    assert(poll(fds, 2, 0) == 1);
    assert(fds[0].fd == s);
    assert(fds[0].events == (POLLIN | POLLOUT));
    assert(fds[0].revents == 0);
    assert(fds[1].fd == c);
    assert(fds[1].events == (POLLIN | POLLOUT));
    assert(fds[1].revents == (POLLIN | POLLOUT));
    
    close(c);
    
    assert(poll(fds, 2, 0) == 1);
    assert(fds[0].fd == s);
    assert(fds[0].events == (POLLIN | POLLOUT));
    assert(fds[0].revents == POLLIN);
    assert(fds[1].fd == c);
    assert(fds[1].events == (POLLIN | POLLOUT));
    assert(fds[1].revents == 0);
    
    close(s);
    return TEST_SUCCESS;
}

int test_passthrough (void) {
    struct pollfd p = (struct pollfd) {
        .fd = 0,
        .events = POLLIN,
        .revents = 0,
    };
    
    assert(poll(&p, 1, 1000) == 0);
    
    return TEST_SUCCESS;
}

test_fn tests [] = {
    test_poll,
    test_passthrough,
    NULL,
};
