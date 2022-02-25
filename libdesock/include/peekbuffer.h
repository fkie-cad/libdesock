#ifndef PEEKBUFFER_H
#define PEEKBUFFER_H

#define STATIC_BUFFER_SIZE 8192

typedef struct {
    char* buffer;
    size_t start;
    size_t size;
    size_t capacity;
} peekbuffer_t;

extern peekbuffer_t peekbuffer;

int peekbuffer_read (size_t len);
size_t peekbuffer_cp (char* dest, size_t len, size_t offset);
size_t peekbuffer_mv (char* dest, size_t len);

inline static size_t peekbuffer_size (void) {
    return peekbuffer.size;
}

#endif
