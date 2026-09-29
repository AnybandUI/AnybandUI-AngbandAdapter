/* Test direct drawing observations without a terminal or child process. */
#include "angband.h"
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "init.h"
#include "monster.h"
#include "mon-make.h"
#include "mon-util.h"
#include "mon-predicate.h"
#include "obj-util.h"
#include "player-birth.h"
#include "player-timed.h"
#include "player-util.h"
#include "trap.h"
#include "ui-map.h"
#include "ui-object.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "../src/anybandui-map.h"

static struct monster *actor;
static struct grid_data observed;
static struct grid_layer observation[MAP_LAYER_MAX];
static int draws;
static void observe(const struct grid_data *g, const struct grid_layer *layers)
{
    observed = *g;
    memcpy(observation, layers, sizeof(observation));
    ++draws;
}

int setup_tests(void **state)
{
    set_file_paths();
    init_angband();
    if (!player_make_simple(NULL, NULL, "Map test")) return 1;
    textui_prefs_init();
    reset_visuals(false);
    cave = t_build_arena(10, 10);
    player_place(cave, player, loc(5, 5));
    actor = t_add_monster(cave, loc(6, 5), "blubbering idiot");
    player->cave = cave_new(10, 10);
    *state = NULL;
    return 0;
}

int teardown_tests(void *state)
{
    map_draw_hook = NULL;
    textui_prefs_free();
    wipe_mon_list(cave, player);
    cleanup_angband();
    return 0;
}

static int test_layers(void *state)
{
    struct grid_data g = {0};
    struct grid_layer layers[MAP_LAYER_MAX] = {{0}};
    struct trap trap = {0};
    int a, ta;
    wchar_t c, tc;
    g.grid = loc(6, 5);
    g.f_idx = FEAT_FLOOR;
    g.lighting = LIGHTING_LIT;
    g.first_kind = &k_info[1];
    g.m_idx = actor->midx;
    trap.kind = &trap_info[1];
    trf_on(trap.flags, TRF_VISIBLE);
    g.trap = &trap;
    grid_data_as_text_layers(&g, &a, &c, &ta, &tc, layers, 1);
    eq(layers[MAP_TERRAIN].attr, ta);
    eq(layers[MAP_TERRAIN].chr, tc);
    eq(layers[MAP_TRAP].attr, trap_x_attr[LIGHTING_LIT][trap.kind->tidx]);
    eq(layers[MAP_TRAP].chr, trap_x_char[LIGHTING_LIT][trap.kind->tidx]);
    eq(layers[MAP_OBJECT].attr, object_kind_attr(g.first_kind));
    eq(layers[MAP_OBJECT].chr, object_kind_char(g.first_kind));
    eq(layers[MAP_ACTOR].attr, a);
    eq(layers[MAP_ACTOR].chr, c);
    map_draw_hook = observe;
    draws = 0;
    grid_data_as_text(&g, &a, &c, &ta, &tc);
    eq(draws, 1);
    require(loc_eq(observed.grid, g.grid));
    eq(observation[MAP_ACTOR].chr, c);
    map_draw_hook = NULL;
    ok;
}

static int test_occlusion(void *state)
{
    struct grid_data g = {0};
    struct grid_layer layers[MAP_LAYER_MAX] = {{0}};
    struct trap trap = {0};
    int a, ta;
    wchar_t c, tc;
    g.f_idx = FEAT_FLOOR;
    g.lighting = LIGHTING_LIT;
    g.first_kind = &k_info[1];
    trap.kind = &trap_info[1];
    trf_on(trap.flags, TRF_WEB);
    g.trap = &trap;
    grid_data_as_text_layers(&g, &a, &c, &ta, &tc, layers, 1);
    eq(layers[MAP_OBJECT].chr, 0);
    eq(layers[MAP_ACTOR].chr, 0);
    eq(layers[MAP_TRAP].chr, c);
    ok;
}

static int test_readonly(void *state)
{
    struct grid_data g = {0};
    struct grid_layer layers[MAP_LAYER_MAX] = {{0}};
    uint32_t rng[RAND_DEG], quick_value = Rand_value;
    bool quick = Rand_quick;
    int a, ta, old_attr = actor->attr;
    wchar_t c, tc;
    memcpy(rng, STATE, sizeof(rng));
    g.f_idx = FEAT_FLOOR;
    g.lighting = LIGHTING_LIT;
    g.m_idx = actor->midx;
    actor->attr = 0;
    grid_data_as_text_layers(&g, &a, &c, &ta, &tc, layers, 7);
    eq(actor->attr, 0);
    g.hallucinate = true;
    g.first_kind = &k_info[1];
    grid_data_as_text_layers(&g, &a, &c, &ta, &tc, layers, 7);
    eq(layers[MAP_ACTOR].chr, L'h');
    eq(layers[MAP_OBJECT].chr, L'!');
    eq(Rand_value, quick_value);
    eq(Rand_quick, quick);
    require(memcmp(rng, STATE, sizeof(rng)) == 0);
    actor->attr = old_attr;
    ok;
}

static int test_known_map(void *state)
{
    struct map_visual visual;
    struct grid_data g;
    struct loc grid = loc(3, 3);
    square_set_feat(player->cave, grid, FEAT_NONE);
    map_info_readonly(grid, &g);
    require(loc_eq(g.grid, grid));
    eq(g.f_idx, FEAT_NONE);
    sqinfo_on(square(cave, grid)->info, SQUARE_SEEN);
    map_visual_readonly(grid, &visual);
    eq(visual.feature, FEAT_NONE);
    eq(square(player->cave, grid)->feat, FEAT_NONE);
    map_info(grid, &g);
    eq(square(player->cave, grid)->feat, FEAT_FLOOR);
    map_visual_readonly(grid, &visual);
    eq(visual.feature, FEAT_FLOOR);
    sqinfo_off(square(cave, grid)->info, SQUARE_SEEN);
    ok;
}

/* Minimap-only lighting overrides must never replace canonical observations. */
static int test_minimap(void *state)
{
    term window;
    int x, y;
    term_init(&window, 80, 24, 16);
    Term_activate(&window);
    map_draw_hook = observe;
    draws = 0;
    display_map(&y, &x);
    eq(draws, cave->width * cave->height);
    map_draw_hook = NULL;
    term_nuke(&window);
    ok;
}

const char *suite_name = "adapter/map";
struct test tests[] = {
    { "layers and direct observer", test_layers },
    { "web occlusion and absent layers", test_occlusion },
    { "read-only RNG and monster colour", test_readonly },
    { "known map and memory isolation", test_known_map },
    { "minimap lighting stays presentation-only", test_minimap },
    { NULL, NULL }
};
