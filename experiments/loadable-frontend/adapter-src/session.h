#ifndef ANYBANDUI_SESSION_H
#define ANYBANDUI_SESSION_H
#include <stdbool.h>
#include <stddef.h>
/* A session owns the engine's global state and runs once on the calling thread.
 * Normal close and birth cancellation return. Callbacks must not reenter it.
 * stop handles fatal transport errors and must not return;
 * Angband also retains process-global fatal-error handling. This is not yet a
 * reusable, multi-session embedding API. */
struct anybandui_io {
	void *user;
	bool (*read_line)(void *, char *, size_t);
	bool (*write_line)(void *, const char *);
	bool (*poll)(void *);
	void (*stop)(void *, int);
};
#ifdef __cplusplus
extern "C" {
#endif
int anybandui_run(const struct anybandui_io *, int argc, char **argv);
#ifdef __cplusplus
}
#endif
#endif
