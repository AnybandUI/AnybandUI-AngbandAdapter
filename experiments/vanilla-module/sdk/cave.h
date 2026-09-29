/**
 * \file cave.h
 * \brief Matters relating to the current dungeon level
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#ifndef CAVE_H
#define CAVE_H

#include "z-type.h"
#include "z-bitflag.h"

struct player;
struct monster;
struct monster_group;

extern __declspec(dllimport) const int16_t ddd[9];
extern __declspec(dllimport) const int16_t ddx[10];
extern __declspec(dllimport) const int16_t ddy[10];
extern __declspec(dllimport) const struct loc ddgrid[10];
extern __declspec(dllimport) const int16_t ddx_ddd[9];
extern __declspec(dllimport) const int16_t ddy_ddd[9];
extern __declspec(dllimport) const struct loc ddgrid_ddd[9];
extern __declspec(dllimport) const int16_t clockwise_ddd[9];
extern __declspec(dllimport) const struct loc clockwise_grid[9];
extern __declspec(dllimport) const int *dist_offsets_y[10];
extern __declspec(dllimport) const int *dist_offsets_x[10];
extern __declspec(dllimport) const uint8_t side_dirs[20][8];

enum {
	DIR_UNKNOWN = 0,
	DIR_NW = 7,
	DIR_N = 8,
	DIR_NE = 9,
	DIR_W = 4,
	DIR_TARGET = 5,
	DIR_NONE = 5,
	DIR_E = 6,
	DIR_SW = 1,
	DIR_S = 2,
	DIR_SE = 3,
};

/**
 * Square flags
 */

enum
{
	#define SQUARE(a,b) SQUARE_##a,
	#include "list-square-flags.h"
	#undef SQUARE
	SQUARE_MAX
};

#define SQUARE_SIZE                FLAG_SIZE(SQUARE_MAX)

#define sqinfo_has(f, flag)        flag_has_dbg(f, SQUARE_SIZE, flag, #f, #flag)
#define sqinfo_next(f, flag)       flag_next(f, SQUARE_SIZE, flag)
#define sqinfo_is_empty(f)         flag_is_empty(f, SQUARE_SIZE)
#define sqinfo_is_full(f)          flag_is_full(f, SQUARE_SIZE)
#define sqinfo_is_inter(f1, f2)    flag_is_inter(f1, f2, SQUARE_SIZE)
#define sqinfo_is_subset(f1, f2)   flag_is_subset(f1, f2, SQUARE_SIZE)
#define sqinfo_is_equal(f1, f2)    flag_is_equal(f1, f2, SQUARE_SIZE)
#define sqinfo_on(f, flag)         flag_on_dbg(f, SQUARE_SIZE, flag, #f, #flag)
#define sqinfo_off(f, flag)        flag_off(f, SQUARE_SIZE, flag)
#define sqinfo_wipe(f)             flag_wipe(f, SQUARE_SIZE)
#define sqinfo_setall(f)           flag_setall(f, SQUARE_SIZE)
#define sqinfo_negate(f)           flag_negate(f, SQUARE_SIZE)
#define sqinfo_copy(f1, f2)        flag_copy(f1, f2, SQUARE_SIZE)
#define sqinfo_union(f1, f2)       flag_union(f1, f2, SQUARE_SIZE)
#define sqinfo_inter(f1, f2)       flag_inter(f1, f2, SQUARE_SIZE)
#define sqinfo_diff(f1, f2)        flag_diff(f1, f2, SQUARE_SIZE)


/**
 * Terrain flags
 */
enum
{
	#define TF(a,b) TF_##a,
	#include "list-terrain-flags.h"
	#undef TF
	TF_MAX
};

#define TF_SIZE                FLAG_SIZE(TF_MAX)

#define tf_has(f, flag)        flag_has_dbg(f, TF_SIZE, flag, #f, #flag)

/**
 * Information about terrain features.
 *
 * At the moment this isn't very much, but eventually a primitive flag-based
 * information system will be used here.
 */
struct feature {
	char *name;
	char *desc;
	int fidx;

	struct feature *mimic;	/**< Feature to mimic or NULL for no mimicry */
	uint8_t priority;	/**< Display priority */

	uint8_t shopnum;	/**< Which shop does it take you to? */
	uint8_t dig;		/**< How hard is it to dig through? */

	bitflag flags[TF_SIZE];	/**< Terrain flags */

	uint8_t d_attr;	/**< Default feature attribute */
	wchar_t d_char;	/**< Default feature character */

	char *walk_msg;	/**< Message on walking into feature */
	char *run_msg;	/**< Message on running into feature */
	char *hurt_msg;	/**< Message on being hurt by feature */
	char *die_msg;	/**< Message on dying to feature */
	char *confused_msg; /**< Message on confused monster move into feature */
	char *look_prefix; /**< Prefix for name in look result */
	char *look_in_preposition; /**< Preposition in look result when on the terrain */
	int resist_flag;/**< Monster resist flag for entering feature */
};

extern __declspec(dllimport) struct feature *f_info;

enum grid_light_level
{
	LIGHTING_LOS = 0,   /* line of sight */
	LIGHTING_TORCH,     /* torchlight */
	LIGHTING_LIT,       /* permanently lit (when not in line of sight) */
	LIGHTING_DARK,      /* dark */
	LIGHTING_MAX
};

struct frontend_grid_data {
	struct loc grid;
	uint32_t m_idx;			/* Monster index */
	uint32_t f_idx;			/* Feature index */
	struct object_kind *first_kind;	/* The kind of the first item on the grid */
	struct trap *trap;		/* Trap */
	bool multiple_objects;	/* Is there more than one item there? */
	bool unseen_object;		/* Is there an unaware object there? */
	bool unseen_money;		/* Is there some unaware money there? */

	enum grid_light_level lighting; /* Light level */
	bool in_view; 			/* Can the player can currently see the grid? */
	bool is_player;
	bool hallucinate;
};

struct square {
	uint8_t feat;
	bitflag *info;
	int light;
	int16_t mon;
	struct object *obj;
	struct trap *trap;
};

struct heatmap {
	uint16_t **grids;
};

struct connector {
	struct loc grid;
	uint8_t feat;
	bitflag *info;
	struct connector *next;
};

struct chunk {
	char *name;
	int32_t turn;
	int depth;

	uint8_t feeling;
	uint32_t obj_rating;
	uint32_t mon_rating;
	bool good_item;

	int height;
	int width;

	uint16_t feeling_squares; /* How many feeling squares the player has visited */
	int *feat_count;

	struct square **squares;
	struct heatmap noise;
	struct heatmap scent;
	struct loc decoy;

	struct object **objects;
	uint16_t obj_max;

	struct monster *monsters;
	uint16_t mon_max;
	uint16_t mon_cnt;
	int mon_current;
	int num_repro;

	struct monster_group **monster_groups;

	struct connector *join;
};

/*** Feature Indexes (see "lib/gamedata/terrain.txt") ***/
enum {
	#define FEAT(x) FEAT_##x,
	#include "list-terrain.h"
	#undef FEAT
	FEAT_MAX
};

/* Current level */
extern __declspec(dllimport) struct chunk *cave;
/* Stored levels */
extern __declspec(dllimport) struct chunk **chunk_list;
extern __declspec(dllimport) uint16_t chunk_list_max;

/* cave-view.c */
__declspec(dllimport) int distance(struct loc grid1, struct loc grid2);
__declspec(dllimport) bool los(struct chunk *c, struct loc grid1, struct loc grid2);
__declspec(dllimport) void update_view(struct chunk *c, struct player *p);
__declspec(dllimport) bool no_light(const struct player *p);

/* cave-map.c */
void map_info(struct loc grid, struct frontend_grid_data *g);
void map_info_readonly(struct loc grid, struct frontend_grid_data *g);
__declspec(dllimport) void square_note_spot(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_light_spot(struct chunk *c, struct loc grid);
__declspec(dllimport) void light_room(struct loc grid, bool light);
__declspec(dllimport) void wiz_light(struct chunk *c, struct player *p, bool full);
__declspec(dllimport) void wiz_dark(struct chunk *c, struct player *p, bool full);
__declspec(dllimport) void cave_illuminate(struct chunk *c, bool daytime);
__declspec(dllimport) void expose_to_sun(struct chunk *c, struct loc grid, bool daytime);
void cave_update_flow(struct chunk *c);
void cave_forget_flow(struct chunk *c);

/* cave-square.c */
/**
 * square_predicate is a function pointer which tests a given square to
 * see if the predicate in question is true.
 */
typedef bool (*square_predicate)(struct chunk *c, struct loc grid);

/* FEATURE PREDICATES */
__declspec(dllimport) bool feat_is_magma(int feat);
__declspec(dllimport) bool feat_is_quartz(int feat);
__declspec(dllimport) bool feat_is_granite(int feat);
__declspec(dllimport) bool feat_is_treasure(int feat);
__declspec(dllimport) bool feat_is_wall(int feat);
__declspec(dllimport) bool feat_is_floor(int feat);
__declspec(dllimport) bool feat_is_trap_holding(int feat);
__declspec(dllimport) bool feat_is_object_holding(int feat);
__declspec(dllimport) bool feat_is_monster_walkable(int feat);
__declspec(dllimport) bool feat_is_shop(int feat);
__declspec(dllimport) bool feat_is_los(int feat);
__declspec(dllimport) bool feat_is_passable(int feat);
__declspec(dllimport) bool feat_is_projectable(int feat);
__declspec(dllimport) bool feat_is_torch(int feat);
__declspec(dllimport) bool feat_is_bright(int feat);
__declspec(dllimport) bool feat_is_fiery(int feat);
__declspec(dllimport) bool feat_is_no_flow(int feat);
__declspec(dllimport) bool feat_is_no_scent(int feat);
__declspec(dllimport) bool feat_is_smooth(int feat);

/* SQUARE FEATURE PREDICATES */
__declspec(dllimport) bool square_isfloor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_istrappable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isobjectholding(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isrock(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isgranite(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isperm(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_ismagma(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isquartz(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_ismineral(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_hasgoldvein(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isrubble(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_issecretdoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isopendoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iscloseddoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isbrokendoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isstairs(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isupstairs(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdownstairs(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isshop(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isplayer(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isoccupied(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isknown(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_ismemorybad(struct chunk *c, struct loc grid);

/* SQUARE INFO PREDICATES */
__declspec(dllimport) bool square_ismark(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isglow(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isvault(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isroom(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isseen(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isview(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_wasseen(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isfeel(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_istrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isinvis(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswall_inner(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswall_outer(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswall_solid(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_ismon_restrict(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isno_teleport(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isno_map(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isno_esp(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isproject(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdtrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isno_stairs(struct chunk *c, struct loc grid);

/* SQUARE BEHAVIOR PREDICATES */
__declspec(dllimport) bool square_isopen(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isempty(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isarrivable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_canputitem(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdiggable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswebbable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_is_monster_walkable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_ispassable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isprojectable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_allowsfeel(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_allowslos(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isstrongwall(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isbright(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isfiery(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_islit(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdamaging(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isnoflow(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isnoscent(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswarded(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdecoyed(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_iswebbed(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_seemslikewall(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isinteresting(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_islockeddoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isunlockeddoor(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isplayertrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isvisibletrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_issecrettrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdisabledtrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isdisarmabletrap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_dtrap_edge(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_changeable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_in_bounds(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_in_bounds_fully(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isbelievedwall(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_isknownpassable(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_suits_stairs_well(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_suits_stairs_ok(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_allows_summon(struct chunk *c, struct loc grid);


__declspec(dllimport) const struct square *square(struct chunk *c, struct loc grid);
__declspec(dllimport) struct feature *square_feat(struct chunk *c, struct loc grid);
__declspec(dllimport) int square_light(struct chunk *c, struct loc grid);
__declspec(dllimport) struct monster *square_monster(struct chunk *c, struct loc grid);
__declspec(dllimport) struct object *square_object(struct chunk *c, struct loc grid);
__declspec(dllimport) struct trap *square_trap(struct chunk *c, struct loc grid);
__declspec(dllimport) bool square_holds_object(struct chunk *c, struct loc grid, struct object *obj);
__declspec(dllimport) void square_excise_object(struct chunk *c, struct loc grid, struct object *obj);
__declspec(dllimport) void square_excise_pile(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_excise_all_imagined(struct chunk *p_c, struct chunk *c,
		struct loc grid);
__declspec(dllimport) void square_delete_object(struct chunk *c, struct loc grid, struct object *obj, bool do_note, bool do_light);
__declspec(dllimport) void square_sense_pile(struct chunk *c, struct loc grid,
		bool (*pred)(const struct object*));
__declspec(dllimport) void square_know_pile(struct chunk *c, struct loc grid,
		bool (*pred)(const struct object*));
__declspec(dllimport) int square_num_walls_adjacent(struct chunk *c, struct loc grid);
__declspec(dllimport) int square_num_walls_diagonal(struct chunk *c, struct loc grid);


/* Feature placers */
__declspec(dllimport) void square_set_feat(struct chunk *c, struct loc grid, int feat);
__declspec(dllimport) void square_set_mon(struct chunk *c, struct loc grid, int midx);
__declspec(dllimport) void square_set_obj(struct chunk *c, struct loc grid, struct object *obj);
__declspec(dllimport) void square_set_trap(struct chunk *c, struct loc grid, struct trap *trap);
__declspec(dllimport) void square_add_trap(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_add_glyph(struct chunk *c, struct loc grid, int type);
__declspec(dllimport) void square_add_web(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_add_stairs(struct chunk *c, struct loc grid, int depth);
__declspec(dllimport) void square_add_door(struct chunk *c, struct loc grid, bool closed);

/* Feature modifiers */
__declspec(dllimport) void square_open_door(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_close_door(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_smash_door(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_unlock_door(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy_door(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy_trap(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_disable_trap(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy_decoy(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_tunnel_wall(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy_wall(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_smash_wall(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_earthquake(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_upgrade_mineral(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_destroy_rubble(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_force_floor(struct chunk *c, struct loc grid);


__declspec(dllimport) int square_shopnum(struct chunk *c, struct loc grid);
__declspec(dllimport) int square_digging(struct chunk *c, struct loc grid);
__declspec(dllimport) const char *square_apparent_name(struct chunk *c, struct loc grid);
__declspec(dllimport) const char *square_apparent_look_prefix(struct chunk *c, struct loc grid);
__declspec(dllimport) const char *square_apparent_look_in_preposition(struct chunk *c, struct loc grid);

__declspec(dllimport) void square_memorize(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_forget(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_mark(struct chunk *c, struct loc grid);
__declspec(dllimport) void square_unmark(struct chunk *c, struct loc grid);

/* cave.c */
__declspec(dllimport) int motion_dir(struct loc source, struct loc target);
__declspec(dllimport) struct loc next_grid(struct loc grid, int dir);
__declspec(dllimport) int lookup_feat(const char *name);
__declspec(dllimport) int lookup_feat_code(const char *code);
__declspec(dllimport) const char *get_feat_code_name(int idx);
__declspec(dllimport) struct chunk *cave_new(int height, int width);
__declspec(dllimport) void cave_connectors_free(struct connector *join);
__declspec(dllimport) void cave_free(struct chunk *c);
__declspec(dllimport) void list_object(struct chunk *c, struct object *obj);
__declspec(dllimport) void delist_object(struct chunk *c, struct object *obj);
__declspec(dllimport) void object_lists_check_integrity(struct chunk *c, struct chunk *c_k);
__declspec(dllimport) void scatter(struct chunk *c, struct loc *place, struct loc grid, int d,
			 bool need_los);
__declspec(dllimport) int scatter_ext(struct chunk *c, struct loc *places, int n, struct loc grid,
		int d, bool need_los, bool (*pred)(struct chunk *, struct loc));

__declspec(dllimport) struct monster *cave_monster(struct chunk *c, int idx);
__declspec(dllimport) int cave_monster_max(struct chunk *c);
__declspec(dllimport) int cave_monster_count(struct chunk *c);

__declspec(dllimport) int count_feats(struct loc *grid,
				bool (*test)(struct chunk *c, struct loc grid), bool under);
__declspec(dllimport) int count_neighbors(struct loc *match, struct chunk *c, struct loc grid,
	bool (*test)(struct chunk *c, struct loc grid), bool under);
__declspec(dllimport) struct loc cave_find_decoy(struct chunk *c);

__declspec(dllimport) void cave_known(struct player *p);

#endif /* !CAVE_H */



