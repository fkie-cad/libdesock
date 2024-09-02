#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>

#include "test_helper.h"

#include "tests.h"

int test_select (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 1) == 0);
    
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    
    FD_SET(s, &rfds);
    FD_SET(s, &wfds);
    FD_SET(1, &wfds);
    
    assert(select(s + 1, &rfds, &wfds, NULL, NULL) == 1);
    assert(FD_ISSET(s, &rfds));
    assert(!FD_ISSET(s, &wfds));
    assert(!FD_ISSET(1, &wfds));
    
    int c = accept(s, NULL, NULL);
    assert(c > s);
    
    FD_SET(s, &rfds);
    FD_SET(s, &wfds);
    FD_SET(c, &rfds);
    FD_SET(c, &wfds);
    
    assert(select(c + 1, &rfds, &wfds, NULL, NULL) == 2);
    assert(FD_ISSET(c, &rfds));
    assert(FD_ISSET(c, &wfds));
    assert(!FD_ISSET(s, &rfds));
    assert(!FD_ISSET(s, &wfds));
    
    close(c);
    
    FD_SET(s, &rfds);
    FD_SET(s, &wfds);
    FD_SET(c, &rfds);
    FD_SET(c, &wfds);
    
    assert(select(c + 1, &rfds, &wfds, NULL, NULL) == 1);
    assert(FD_ISSET(s, &rfds));
    assert(!FD_ISSET(s, &wfds));
    assert(!FD_ISSET(c, &rfds));
    assert(!FD_ISSET(c, &wfds));
    
    close(s);
    
    return TEST_SUCCESS;
}

int test_passthrough (void) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    
    struct timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0,
    };
    
    assert(select(2, &rfds, NULL, NULL, &tv) == 0);
    
    return TEST_SUCCESS;
}

test_fn tests [] = {
    test_select,
    test_passthrough,
    NULL,
};
