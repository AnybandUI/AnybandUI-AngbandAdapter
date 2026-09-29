/* Official backend checks. SDL_AUDIODRIVER=dummy enables headless SDL2 tests. */
#include "angband.h"
#include "unit-test.h"
#include "test-utils.h"
#include "init.h"
#include "message.h"
#include "option.h"
#include "player.h"
#include "sound.h"
#ifdef SOUND_SDL2
#include "snd-sdl.h"
#include "SDL.h"
#define SOUND_BACKEND "sdl"
#define init_backend init_sound_sdl
#else
#include "snd-win.h"
#define SOUND_BACKEND "win"
#define init_backend init_sound_win
#endif
#include "ui-prefs.h"

static struct sound_hooks backend;
static struct sound_data sample;

int setup_tests(void **state)
{
    set_file_paths();
    init_angband();
    textui_prefs_init();
    *state = NULL;
    return init_backend(&backend, 0, NULL);
}

int teardown_tests(void *state)
{
    backend.unload_sound_hook(&sample);
    close_sound();
    textui_prefs_free();
    cleanup_angband();
    return 0;
}

static int test_mp3(void *state)
{
    char path[1024];
    const struct sound_file_type *type = backend.supported_files_hook();
    while (type->type && !streq(type->extension, ".mp3")) ++type;
    require(type->type);
    require(backend.open_audio_hook());
    path_build(path, sizeof(path), ANGBAND_DIR_SOUNDS, "plm_eat_bite.mp3");
    require(backend.load_sound_hook(path, type->type, &sample));
    eq(sample.status, SOUND_ST_LOADED);
    require(backend.play_sound_hook(&sample));
    require(backend.unload_sound_hook(&sample));
    null(sample.plat_data);
    require(backend.close_audio_hook());
    ok;
}

static int test_missing_file(void *state)
{
    const struct sound_file_type *type = backend.supported_files_hook();
    while (type->type && !streq(type->extension, ".mp3")) ++type;
    require(type->type);
    require(!backend.load_sound_hook("nonexistent-sound-test.mp3", type->type, &sample));
    null(sample.plat_data);
    ok;
}

static int test_empty_restart(void *state)
{
    for (int i = 0; i < 2; ++i) {
        eq(init_sound(SOUND_BACKEND, 0, NULL), 0);
        require(is_sound_inited());
        close_sound();
        close_sound();
        require(!is_sound_inited());
#ifdef SOUND_SDL2
        eq(SDL_WasInit(SDL_INIT_AUDIO), 0);
#endif
    }
    ok;
}

static int test_preferences_and_restart(void *state)
{
    int i;
    for (i = 0; i < 2; ++i) {
        eq(init_sound(SOUND_BACKEND, 0, NULL), 0);
        require(process_pref_file("sound.prf", false, false));
        require(is_sound_inited());
        player->opts.opt[OPT_use_sound] = false;
        sound(MSG_EAT);
        player->opts.opt[OPT_use_sound] = true;
        sound(MSG_EAT);
        close_sound();
        require(!is_sound_inited());
        /* No dangling playback handler may read the freed sound mappings. */
        player->opts.opt[OPT_use_sound] = true;
        sound(MSG_EAT);
    }
    ok;
}

const char *suite_name = "adapter/sound";
struct test tests[] = {
    { "official MP3 load/play/unload", test_mp3 },
    { "missing sample", test_missing_file },
    { "startup without samples and repeated close", test_empty_restart },
    { "official preferences and restart", test_preferences_and_restart },
    { NULL, NULL }
};
