#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <unistd.h>

#include "test_helper.h"

#include "tests.h"

int test_sockaddr_in (void) {
    struct sockaddr_in in;
    socklen_t in_size = sizeof(in);
    
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 0) == 0);
    int c = accept(s, (struct sockaddr*) &in, &in_size);
    assert(c > s);
    
    assert(in.sin_family == AF_INET);
    assert(in.sin_port == 53764);
    assert(in.sin_addr.s_addr == 0x100007f);
    assert(in_size == sizeof(in));
    
    assert(close(c) == 0);
    assert(close(s) == 0);
    
    return TEST_SUCCESS;
}

int test_sockaddr_in6 (void) {
    struct sockaddr_in6 in;
    socklen_t in_size = sizeof(in);
    
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    assert(s > 2);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 0) == 0);
    int c = accept(s, (struct sockaddr*) &in, &in_size);
    assert(c > s);
    
    assert(in.sin6_family == AF_INET6);
    assert(in.sin6_port == 53764);
    assert(in.sin6_flowinfo == 0);
    for (int i = 0; i < 15; ++i) {
        assert(in.sin6_addr.s6_addr[i] == 0);
    }
    assert(in.sin6_addr.s6_addr[15] == 1);
    assert(in.sin6_scope_id == 0);
    assert(in_size == sizeof(in));
    
    assert(close(c) == 0);
    assert(close(s) == 0);
    
    return TEST_SUCCESS;
}

int test_out_of_fds (void) {
    struct sockaddr_in6 in;
    socklen_t in_size = sizeof(in);
    
    while (dup(0) < __libdesock_fd_table_size());

    int target_fd = __libdesock_fd_table_size() - 1;
    close(target_fd);
    
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    assert(s == target_fd);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 0) == 0);
    int c = accept(s, (struct sockaddr*) &in, &in_size);
    assert(c == -1);
    
    close(s);
    return TEST_SUCCESS;
}

test_fn tests [] = {
    test_sockaddr_in,
    test_sockaddr_in6,
    test_out_of_fds,
    NULL,
};
