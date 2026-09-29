/* Test-only bridge: keep the existing integration scenarios unchanged. */
#include "adapter-src/session.h"
#include "frontend.h"
#include <stdlib.h>
int anybandui_run(const struct anybandui_io *io, int argc, char **argv)
{
    const struct frontend_io stream = {
        io->user, io->read_line, io->write_line, io->poll, io->stop
    };
    return frontend_run(getenv("ANGBAND_TEST_FRONTEND"), &stream, argc, argv);
}
