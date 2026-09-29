#include "engine-api.h"
#include "frontend.h"
#include <build-id.h>
#include "cJSON.h"
#include "adapter-src/session.h"
#include <locale.h>
#include <sys/stat.h>
#include <windows.h>
static const struct frontend_engine_api *frontend_engine_ptr;
/* These engine macros expand their dependencies after adapter compilation. */
#define flag_has (*frontend_engine_ptr->p_flag_has)
#define row_top_map (*frontend_engine_ptr->p_row_top_map)
#define row_bottom_map (*frontend_engine_ptr->p_row_bottom_map)
#define col_map (*frontend_engine_ptr->p_col_map)
#define Term (*frontend_engine_ptr->p_Term)
#define tile_width (*frontend_engine_ptr->p_tile_width)
#define tile_height (*frontend_engine_ptr->p_tile_height)
#include "adapter-src/main-anybandui.c"
static int plugin_run(const struct frontend_engine_api *api,
                      const struct frontend_io *io, int argc, char **argv)
{
    const struct anybandui_io stream = {
        io->user, io->read_line, io->write_line, io->poll, io->stop
    };
    frontend_engine_ptr = api;
    return anybandui_run(&stream, argc, argv);
}
__declspec(dllexport) const struct frontend_module *angband_frontend_v1(void)
{
    static const struct frontend_module module = {
        1, FRONTEND_TABLE_ID, sizeof(struct frontend_engine_api), plugin_run
    };
    return &module;
}
