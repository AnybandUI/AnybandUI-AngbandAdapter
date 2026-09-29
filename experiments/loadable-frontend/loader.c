#include "frontend.h"
#include "engine-api.h"
#include <build-id.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
extern const struct frontend_engine_api frontend_engine;
int frontend_run(const char *path, const struct frontend_io *io,
                 int argc, char **argv)
{
    /* One selected module for the process lifetime: callbacks may remain set. */
    static HMODULE library;
    static const struct frontend_module *module;
    if (!library) {
        wchar_t wide[MAX_PATH];
        frontend_query_fn query;
        if (!path || !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                    path, -1, wide, MAX_PATH)) return 70;
        library = LoadLibraryExW(wide, NULL,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!library) {
            fprintf(stderr, "Cannot load frontend (%lu).\n", GetLastError());
            return 71;
        }
        query = (frontend_query_fn)GetProcAddress(library, "angband_frontend_v1");
        module = query ? query() : NULL;
        if (!module || module->version != 1 || !module->build_id ||
                strcmp(module->build_id, FRONTEND_TABLE_ID) ||
                module->api_size != sizeof(frontend_engine) || !module->run) {
            fprintf(stderr, "Frontend does not match this engine build.\n");
            FreeLibrary(library);
            library = NULL;
            module = NULL;
            return 72;
        }
    }
    return module->run(&frontend_engine, io, argc, argv);
}
