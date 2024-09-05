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

test_fn tests [] = {
    test_multi_simple,
    test_multi_simple_fucked_up,
    test_multi_partial,
    NULL,
};
