#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "test_helper.h"
#include "tests.h"

int test_multi_simple (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(connect(s, NULL, 0) == 0);
    
    stdin_file_create("AAAA---BBBB---");
    
    char buf[256];
    
    assert(read(s, buf, sizeof(buf)) == 4);
    assert(memcmp(buf, "AAAA", 4) == 0);
    
    assert(read(s, buf, sizeof(buf)) == 4);
    assert(memcmp(buf, "BBBB", 4) == 0);
    
    assert(read(s, buf, sizeof(buf)) == 0);
    
    stdin_file_destroy();
    close(s);
    return TEST_SUCCESS;
}

int test_multi_simple_fucked_up (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(connect(s, NULL, 0) == 0);
    
    stdin_file_create("AAAA---BBBB---");
    
    char buf[256];
    
    assert(read(s, buf, 4) == 4);
    assert(memcmp(buf, "AAAA", 4) == 0);
    
    assert(read(s, buf, sizeof(buf)) == 0);
    
    assert(read(s, buf, 4) == 4);
    assert(memcmp(buf, "BBBB", 4) == 0);

    assert(read(s, buf, sizeof(buf)) == 0);
    assert(read(s, buf, sizeof(buf)) == 0);
    
    stdin_file_destroy();
    close(s);
    return TEST_SUCCESS;
}

int test_multi_partial (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(connect(s, NULL, 0) == 0);
    
    stdin_file_create("AXYZ---B12---CD-----");
    
    char buf[256];
    
    assert(read(s, buf, 5) == 4);
    assert(memcmp(buf, "AXYZ", 4) == 0);
    
    assert(read(s, buf, 5) == 3);
    assert(memcmp(buf, "B12", 3) == 0);

    assert(read(s, buf, 2) == 2);
    assert(memcmp(buf, "CD", 2) == 0);

    assert(read(s, buf, sizeof(buf)) == 0);
    
    assert(read(s, buf, sizeof(buf)) == 2);
    assert(memcmp(buf, "--", 2) == 0);
    
    assert(read(s, buf, sizeof(buf)) == 0);
    
    stdin_file_destroy();
    close(s);
    return TEST_SUCCESS;
}

int test_multi_peek (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(connect(s, NULL, 0) == 0);

    stdin_file_create("12345678---ABC---");

    char buf[256];

    // Peek only for the next packet, never beyond
    for (size_t i = 1; i < 8; ++i) {
        assert(recv(s, buf, i, MSG_PEEK) == (ssize_t) i);
        assert(memcmp(buf, "12345678", i) == 0);
    }

    assert(recv(s, buf, 9, MSG_PEEK) == 8);
    assert(recv(s, buf, 10, MSG_PEEK) == 8);
    assert(recv(s, buf, 11, MSG_PEEK) == 8);
    
    assert(recv(s, buf, sizeof(buf), MSG_PEEK) == 8);

    // Release peekbuffer
    assert(recv(s, buf, sizeof(buf), 0) == 8);

    // Peek but without locking peekbuffer
    assert(recv(s, buf, 2, MSG_PEEK) == 2);
    assert(memcmp(buf, "AB", 2) == 0);
    
    assert(recv(s, buf, 4, 0) == 3);
    assert(memcmp(buf, "ABC", 2) == 0);
    
    assert(recv(s, buf, sizeof(buf), 0) == 0);
    
    stdin_file_destroy();
    close(s);
    return TEST_SUCCESS;
}

test_fn tests [] = {
    test_multi_simple,
    test_multi_simple_fucked_up,
    test_multi_partial,
    test_multi_peek,
    NULL,
};
