#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

extern test_fn tests[];

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
