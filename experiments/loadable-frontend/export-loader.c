#include "export-api.h"
#include <build-id.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
static void *lookup(const char *name)
{
    return (void *)GetProcAddress(GetModuleHandleW(NULL), name);
}
int export_frontend_run(const char *path, const struct frontend_io *io,
                        int argc, char **argv)
{
    static HMODULE library;
    static const struct exported_frontend *module;
    static const struct frontend_host host = { lookup };
    if (!library) {
        wchar_t wide[MAX_PATH];
        exported_frontend_query query;
        if (!path || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                    path, -1, wide, MAX_PATH)) return 70;
        library = LoadLibraryExW(wide, NULL,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!library) {
            fprintf(stderr, "Cannot load frontend (%lu).\n", GetLastError());
            return 71;
        }
        query = (exported_frontend_query)GetProcAddress(library, "angband_frontend_v1");
        module = query ? query() : NULL;
        if (!module || module->version != 1 || !module->build_id ||
                strcmp(module->build_id, FRONTEND_BUILD_ID) ||
                module->host_size != sizeof(host) || !module->run) {
            fprintf(stderr, "Frontend does not match this engine build.\n");
            FreeLibrary(library);
            library = NULL;
            module = NULL;
            return 72;
        }
    }
    return module->run(&host, io, argc, argv);
}
