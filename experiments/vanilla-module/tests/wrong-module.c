#include "frontend-module.h"
static int rejected_run(int argc, char **argv) { return 99; }
__declspec(dllexport) const struct frontend_module *angband_frontend_v1(void) {
    static const struct frontend_module module = {"deliberately-incompatible-build", rejected_run};
    return &module;
}
