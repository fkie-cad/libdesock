/*
 *  Support for multiple requests in a single input file by separating them with a given delimiter.
 *  Inspired by Kelly Patterson - Cisco Talos
 */

#include <stddef.h>
#include <string.h>

#include "util.h"
#include "desock.h"
#include "hooks.h"
#include "multi.h"

#ifdef MULTI_REQUEST
#define DELIMITER_LEN (sizeof(REQUEST_DELIMITER) - 1)

static int is_partial_delimiter (char* start, size_t len) {
    char buf[DELIMITER_LEN] = {0};
    memcpy(buf, start, len);
    
    ssize_t rem_len = DELIMITER_LEN - len;
    ssize_t n = hook_input(&buf[len], rem_len);
    
    if (n <= 0) {
        return 0;
    } else if (n != rem_len) {
        if (UNLIKELY(hook_seek(-n) == -1)) {
            _error("lseek on stdin failed when trying to handle partial request delimiter");
        }
        return 0;
    }
    
    if (!__builtin_memcmp(buf, REQUEST_DELIMITER, DELIMITER_LEN)) {
        return 1;
    } else {
        if (UNLIKELY(hook_seek(-n) == -1)) {
            _error("lseek on stdin failed when trying to handle partial request delimiter");
        }
        return 0;
    }
}
#endif

ssize_t postprocess_input (char* buf, ssize_t size) {
#ifndef MULTI_REQUEST
    (void) buf;
#else
    if (size < (ssize_t) DELIMITER_LEN) {
        return size;
    }

    /* Search first occurence of delimiter in input */
    for (size_t i = 0; i <= size - DELIMITER_LEN; ++i) {
        if (!__builtin_memcmp(&buf[i], REQUEST_DELIMITER, DELIMITER_LEN)) {
            if (hook_seek(-(size - i - DELIMITER_LEN)) == -1) {
                _error("lseek on stdin failed when trying to handle request delimiter");
            }
            return i;
        }
    }
    
    /* Search for partial delimiter at the end of input */
    for (ssize_t i = size - DELIMITER_LEN + 1; i < size; ++i) {
        if (!__builtin_memcmp(&buf[i], REQUEST_DELIMITER, size - i) && is_partial_delimiter(&buf[i], size - i)) {
            return i;
        }
    }
#endif

    return size;
}
