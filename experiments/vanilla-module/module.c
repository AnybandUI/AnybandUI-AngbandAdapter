#include "angband.h"
#include "obj-power.h"
#include "frontend-module.h"
#include "frontend-build.h"
#include "src/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#ifdef FRONTEND_SESSION_CHECK
int frontend_session_test(int, char **);
#endif
#ifdef FRONTEND_CLIENT_CHECK
int frontend_client_test(int, char **);
#endif
static bool read_line(void *user, char *line, size_t size) { return fgets(line, (int)size, stdin) != NULL; }
static bool write_line(void *user, const char *line) { return fputs(line, stdout) >= 0 && fputc('\n', stdout) != EOF && fflush(stdout) == 0; }
static bool poll(void *user) {
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD available;
    return PeekNamedPipe(input, NULL, 0, NULL, &available, NULL) && available;
}
static void stop(void *user, int status) { exit(status); }
static int run(int argc, char **argv) {
    const struct anybandui_io io = {NULL, read_line, write_line, poll, stop};
#ifdef FRONTEND_CLIENT_CHECK
    if (getenv("ANYBANDUI_CLIENT_CHECK")) return frontend_client_test(argc, argv);
#endif
#ifdef FRONTEND_SESSION_CHECK
    if (getenv("ANYBANDUI_MODULE_TEST")) return frontend_session_test(argc, argv);
#endif
    return anybandui_run(&io, argc, argv);
}
__declspec(dllexport) const struct frontend_module *angband_frontend_v1(void) {
    static const struct frontend_module module = {ANGBAND_FRONTEND_BUILD_ID, run};
    return &module;
}




