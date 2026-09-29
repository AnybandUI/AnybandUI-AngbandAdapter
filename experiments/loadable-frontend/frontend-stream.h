#ifndef FRONTEND_STREAM_H
#define FRONTEND_STREAM_H
#include <stddef.h>
#include <stdbool.h>
struct frontend_io {
    void *user;
    bool (*read_line)(void *, char *, size_t);
    bool (*write_line)(void *, const char *);
    bool (*poll)(void *);
    void (*stop)(void *, int);
};
#endif
