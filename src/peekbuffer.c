#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "desock.h"
#include "hooks.h"
#include "multi.h"
#include "peekbuffer.h"

typedef struct {
    char* buffer;
    size_t start;
    size_t size;
    size_t capacity;
} peekbuffer_t;

#define STATIC_BUFFER_SIZE 1048576

static char static_buffer[STATIC_BUFFER_SIZE];

static peekbuffer_t peekbuffer = {
    .buffer = static_buffer,
    .start = 0,
    .size = 0,
    .capacity = STATIC_BUFFER_SIZE
};

static size_t next_pow_2 (size_t size) {
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size |= size >> 32;
    size++;
    return size;
}

static int peekbuffer_grow (size_t new_size) {
    if (peekbuffer.start + new_size > peekbuffer.capacity) {
        if (new_size <= peekbuffer.capacity) {
            memmove(peekbuffer.buffer, peekbuffer.buffer + peekbuffer.start, peekbuffer.size);
            peekbuffer.start = 0;
        } else {
            size_t capacity = next_pow_2(new_size);
            DEBUG_LOG("Reallocating peekbuffer to size %lu", capacity);
            
            char* buffer = malloc(capacity);

            if (UNLIKELY(NULL == buffer)) {
                errno = ENOMEM;
                return -1;
            }

            memcpy(buffer, peekbuffer.buffer + peekbuffer.start, peekbuffer.size);

            if (peekbuffer.buffer != static_buffer) {
                free(peekbuffer.buffer);
            }

            peekbuffer.start = 0;
            peekbuffer.buffer = buffer;
            peekbuffer.capacity = capacity;
        }
    }

    return 0;
}

ssize_t peekbuffer_read (size_t len) {
    DEBUG_LOG("peekbuffer_read(%lu)", len);

    if (UNLIKELY(peekbuffer_grow(peekbuffer.size + len) == -1)) {
        return -1;
    }

    ssize_t ret = hook_input(peekbuffer.buffer + peekbuffer.start + peekbuffer.size, len);

    if (LIKELY(ret >= 0)) {
        ret = postprocess_input(peekbuffer.buffer + peekbuffer.start + peekbuffer.size, ret);
        peekbuffer.size += ret;
    }

    DEBUG_LOG(" => %ld", ret);
    return ret;
}

size_t peekbuffer_cp (char* dest, size_t len, size_t offset) {
    DEBUG_LOG("peekbuffer_cp(%p, %lu, %lu)", dest, len, offset);

    if (UNLIKELY(offset >= peekbuffer.size)) {
        return 0;
    }

    len = MIN(len, peekbuffer.size - offset);
    memcpy(dest, peekbuffer.buffer + peekbuffer.start + offset, len);
    DEBUG_LOG(" => %lu", len);
    return len;
}

size_t peekbuffer_mv (char* dest, size_t len) {
    DEBUG_LOG("peekbuffer_mv(%p, %lu)", dest, len);

    size_t ret = peekbuffer_cp(dest, len, 0);
    peekbuffer.size -= ret;

    if (peekbuffer.size == 0) {
        peekbuffer.start = 0;
    } else {
        peekbuffer.start += ret;
    }

    return ret;
}

size_t peekbuffer_size (void) {
    return peekbuffer.size;
}
