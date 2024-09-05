#pragma once

typedef int (*test_fn)(void);

#define TEST_SUCCESS 1
#define TEST_FAILURE 0

void stdin_file_create (char* buf);
void stdin_file_destroy (void);
