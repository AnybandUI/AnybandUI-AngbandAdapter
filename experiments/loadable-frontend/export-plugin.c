#define angband_frontend_v1 table_frontend_unused
#include "plugin.c"
#undef angband_frontend_v1
#include "export-api.h"
static int export_run(const struct frontend_host *host, const struct frontend_io *io,
                       int argc, char **argv)
{
    static struct frontend_engine_api api;
    /* Ordinary lookup of intentionally exported symbols; no address scanning. */
#define FRONTEND_SYMBOL(name) \
    api.p_##name = (typeof(api.p_##name))host->lookup(#name); \
    if (!api.p_##name) { fprintf(stderr, "Missing engine symbol: %s\n", #name); return 73; }
#include "engine-symbols.inc"
#undef FRONTEND_SYMBOL
    return plugin_run(&api, io, argc, argv);
}
__declspec(dllexport) const struct exported_frontend *angband_frontend_v1(void)
{
    static const struct exported_frontend module = {
        1, FRONTEND_BUILD_ID, sizeof(struct frontend_host), export_run
    };
    return &module;
}
