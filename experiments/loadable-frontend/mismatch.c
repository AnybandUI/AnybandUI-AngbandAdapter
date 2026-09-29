#include "engine-api.h"
#include "frontend.h"
#include <build-id.h>
static int should_not_run(const struct frontend_engine_api *api,
        const struct frontend_io *io, int argc, char **argv) { return 99; }
__declspec(dllexport) const struct frontend_module *angband_frontend_v1(void)
{
    static const struct frontend_module module = {
#ifdef WRONG_SIZE
#ifdef EXPORT_VARIANT
        1, FRONTEND_BUILD_ID, 0, should_not_run
#else
        1, FRONTEND_TABLE_ID, 0, should_not_run
#endif
#else
        1, "incompatible-build", sizeof(struct frontend_engine_api), should_not_run
#endif
    };
    return &module;
}
