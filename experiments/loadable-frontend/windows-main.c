/* Windows entry integration experiment. Normal startup remains available. */
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
int WINAPI default_WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
int main(int, char **); /* Existing native scenario, test build only. */
static bool read_line(void *user, char *line, size_t size)
{
    return fgets(line, (int)size, stdin) != NULL;
}
static bool write_line(void *user, const char *line)
{
    return puts(line) >= 0 && fflush(stdout) == 0;
}
static bool poll_input(void *user)
{
    DWORD n = 0;
    return PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &n, NULL) && n;
}
static void stop(void *user, int status) { exit(status); }
int WINAPI WinMain(HINSTANCE current, HINSTANCE previous, LPSTR command, int show)
{
    int argc, result, i;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv;
    const struct frontend_io io = { NULL, read_line, write_line, poll_input, stop };
    if (!wide) return 70;
    if (argc < 2 || (wcscmp(wide[1], L"--frontend") &&
                    wcscmp(wide[1], L"--frontend-test"))) {
        LocalFree(wide);
        return default_WinMain(current, previous, command, show);
    }
    if (argc < 3) { LocalFree(wide); return 70; }
    argv = calloc((size_t)argc + 1, sizeof(*argv));
    if (!argv) { LocalFree(wide); return 70; }
    for (i = 0; i < argc; ++i) {
        int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                wide[i], -1, NULL, 0, NULL, NULL);
        argv[i] = size ? malloc((size_t)size) : NULL;
        if (!argv[i] || !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                wide[i], -1, argv[i], size, NULL, NULL)) {
            while (i >= 0) free(argv[i--]);
            free(argv); LocalFree(wide); return 70;
        }
    }
    LocalFree(wide);
    setvbuf(stdin, NULL, _IONBF, 0);
    if (!strcmp(argv[1], "--frontend-test")) {
        /* Only the prototype contains this fixture route. */
        _putenv_s("ANGBAND_TEST_FRONTEND", argv[2]);
        result = main(argc - 2, argv + 2);
    } else {
        result = frontend_run(argv[2], &io, argc - 2, argv + 2);
    }
    for (i = 0; i < argc; ++i) free(argv[i]);
    free(argv);
    return result;
}
