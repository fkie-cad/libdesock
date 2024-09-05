#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#include "test_helper.h"
#include "tests.h"

int counter = 0;

void* thread_handler (void* arg) {
    int fd = (int)(long) arg;
    assert(fd == 4); // this is the check for synchronization
    usleep(1000000);
    close(fd);
    return NULL;
}

int test_multithreading (void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s > 2);
    assert(bind(s, NULL, 0) == 0);
    assert(listen(s, 0) == 0);

    pthread_t threads[10] = {0};

    for (int i = 0; i < 10; ++i) {
        int c = accept(s, NULL, NULL);
        assert(pthread_create(&threads[i], NULL, thread_handler, (void*)(long) c) == 0);
    }

    for (int i = 0; i < 10; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
    }
    
    return TEST_SUCCESS;
}

test_fn tests[] = {
    test_multithreading,
    NULL,
};
