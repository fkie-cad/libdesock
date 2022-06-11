#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "desock.h"
#include "syscall.h"
#include "peekbuffer.h"

char static_buffer[STATIC_BUFFER_SIZE];

peekbuffer_t peekbuffer = {
    .buffer = static_buffer,
    .start = 0,
    .size = 0,
    .capacity = STATIC_BUFFER_SIZE
};

/*
Round `size` up to the next power of 2.
*/
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
            memmove (peekbuffer.buffer, peekbuffer.buffer + peekbuffer.start, peekbuffer.size);
            peekbuffer.start = 0;
        } else {
            size_t capacity = next_pow_2 (new_size);
            char* buffer = malloc (capacity);

            if (NULL == buffer) {
                errno = ENOMEM;
                return -1;
            }

            memcpy (buffer, peekbuffer.buffer + peekbuffer.start, peekbuffer.size);

            if (peekbuffer.buffer != static_buffer) {
                free (peekbuffer.buffer);
            }

            peekbuffer.start = 0;
            peekbuffer.buffer = buffer;
            peekbuffer.capacity = capacity;
        }
    }

    return 0;
}

int peekbuffer_read (size_t len) {
    DEBUG_LOG ("[%d] desock::peekbuffer_read(%lu)", gettid (), len);

    if (peekbuffer_grow (peekbuffer.size + len) == -1) {
        DEBUG_LOG (" = -1\n");
        return -1;
    }

    int ret = syscall_cp (SYS_read, 0, peekbuffer.buffer + peekbuffer.start + peekbuffer.size, len);

    if (ret > 0) {
        peekbuffer.size += ret;
    }

    DEBUG_LOG (" = %d\n", ret);
    return ret;
}

size_t peekbuffer_cp (char* dest, size_t len, size_t offset) {
    DEBUG_LOG ("[%d] desock::peekbuffer_cp(%p, %lu, %lu)", gettid (), dest, len, offset);

    if (offset >= peekbuffer.size) {
        DEBUG_LOG (" = 0\n");
        return 0;
    }

    len = MIN (len, peekbuffer.size - offset);
    memcpy (dest, peekbuffer.buffer + peekbuffer.start + offset, len);
    DEBUG_LOG (" = %lu\n", len);
    return len;
}

size_t peekbuffer_mv (char* dest, size_t len) {
    DEBUG_LOG ("[%d] desock::peekbuffer_mv(%p, %lu) -> ", gettid (), dest, len);

    size_t ret = peekbuffer_cp (dest, len, 0);
    peekbuffer.size -= ret;

    if (peekbuffer.size == 0) {
        peekbuffer.start = 0;
    } else {
        peekbuffer.start += ret;
    }

    return ret;
}
