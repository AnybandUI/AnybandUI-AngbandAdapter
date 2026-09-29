#ifndef FRONTEND_MODULE_H
#define FRONTEND_MODULE_H
#include "frontend-stream.h"
struct frontend_engine_api;
struct frontend_module {
    unsigned version;
    const char *build_id;
    size_t api_size;
    int (*run)(const struct frontend_engine_api *, const struct frontend_io *,
               int, char **);
};
typedef const struct frontend_module *(*frontend_query_fn)(void);
int frontend_run(const char *, const struct frontend_io *, int, char **);
#endif
