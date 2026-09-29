#ifndef FRONTEND_EXPORT_API_H
#define FRONTEND_EXPORT_API_H
#include "frontend-stream.h"
/* Local, native, matching-build interface; no cross-version ABI promise. */
struct frontend_host {
    void *(*lookup)(const char *name);
};
struct exported_frontend {
    unsigned version;
    const char *build_id;
    size_t host_size;
    int (*run)(const struct frontend_host *, const struct frontend_io *, int, char **);
};
typedef const struct exported_frontend *(*exported_frontend_query)(void);
int export_frontend_run(const char *, const struct frontend_io *, int, char **);
#endif
