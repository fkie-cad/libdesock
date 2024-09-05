#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "tests.h"

extern test_fn tests[];

static int stdin_backup = -1;

void stdin_file_create (char* buf) {
    size_t len = strlen(buf);
    
    stdin_backup = dup(0);
    if (-1 == stdin_backup) {
        abort();
    }
    
    int file = open("/tmp/input", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG);
    if (-1 == file) {
        abort();
    }
    
    if ((size_t) write(file, buf, len) != len) {
        abort();
    }
    
    if (lseek(file, 0, SEEK_SET) != 0) {
        abort();
    }
    
    dup2(file, 0);
    close(file);
}

void stdin_file_destroy (void) {
    if (stdin_backup != -1) {
        close(0);
        dup2(stdin_backup, 0);
        close(stdin_backup);
        stdin_backup = -1;
    }
}

int main (int argc, char** argv) {
    (void) argc;
    (void) argv;
    
    for (unsigned int i = 0; tests[i]; ++i) {
        printf("Running test #%u\n", i + 1);
        
        if (tests[i]() != TEST_SUCCESS) {
            abort();
        }
    }
}
