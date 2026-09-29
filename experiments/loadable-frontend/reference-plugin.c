/* Independent client of the generic plugin interface; no AnybandUI code. */
#include "export-api.h"
#include "game-event.h"
#include <build-id.h>
#include <stdio.h>
static void observed(game_event_type type, game_event_data *data, void *user)
{
    if (type == EVENT_GOLD) ++*(int *)user;
}
static int run(const struct frontend_host *host, const struct frontend_io *io,
                int argc, char **argv)
{
    void (*add)(game_event_type, game_event_handler *, void *) =
        (void (*)(game_event_type, game_event_handler *, void *))host->lookup("event_add_handler");
    void (*remove)(game_event_type, game_event_handler *, void *) =
        (void (*)(game_event_type, game_event_handler *, void *))host->lookup("event_remove_handler");
    void (*signal)(game_event_type) = (void (*)(game_event_type))host->lookup("event_signal");
    int count = 0;
    if (!add || !remove || !signal) return 73;
    add(EVENT_GOLD, observed, &count);
    signal(EVENT_GOLD);
    remove(EVENT_GOLD, observed, &count);
    signal(EVENT_GOLD);
    if (count != 1) return 74;
    if (io && io->write_line)
        return io->write_line(io->user, "PASS: independent frontend used stock event callbacks") ? 0 : 75;
    puts("PASS: independent frontend used stock event callbacks");
    return 0;
}
__declspec(dllexport) const struct exported_frontend *angband_frontend_v1(void)
{
    static const struct exported_frontend module = {
        1, FRONTEND_BUILD_ID, sizeof(struct frontend_host), run
    };
    return &module;
}
