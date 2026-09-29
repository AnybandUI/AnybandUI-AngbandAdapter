#include "init.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-info.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "store.h"
#include "ui-map.h"
#include "ui-input.h"
#include "ui-player.h"
#include "ui-prefs.h"
#include "ui-object.h"
#include "trap.h"
#include "z-textblock.h"

static void document_contract(void)
{
	textblock *a = textblock_new(), *b = textblock_new(), *sheet;
	const struct textblock_span *spans;
	size_t n, i, covered = 0;
	textblock_append(a, "Original wording.\n");
	textblock_end_section(a, "lore");
	textblock_append_field(a, "status", "HP", "20/20", 3);
	textblock_append(b, "prefix\n");
	textblock_append_textblock(b, a);
	textblock_free(a);
	spans = textblock_spans(b, &n);
	check(n == 2 && spans[0].start == 7, "document concatenation shifts spans");
	check(!strcmp(spans[1].label, "HP") && !strcmp(spans[1].value, "20/20"),
	      "copied document owns field strings");
	check(wcscmp(textblock_text(b),
	             L"prefix\nOriginal wording.\nHP: 20/20\n") == 0,
	      "structured text remains usable as ordinary text");
	textblock_free(b);
	sheet = character_sheet();
	spans = textblock_spans(sheet, &n);
	check(n > 25, "character document contains all panels");
	for (i = 0; i < n; ++i) {
		check(spans[i].start == covered,
		      "character fields cover contiguous text");
		covered += spans[i].length;
	}
	check(covered == wcslen(textblock_text(sheet)),
	      "character document spans cover text");
	check(!strcmp(spans[0].value, player->full_name),
	      "character document preserves full name");
	textblock_free(sheet);
	{
		int attr;
		wchar_t ch;
		Term_save();
		display_player_xtra_info();
		Term_what(1, 1, &attr, &ch);
		check(ch == L'N',
		      "terminal renderer consumes the shared character document");
		Term_load();
	}
	if (player->gear) {
		textblock *description = object_info(player->gear, OINFO_NONE);
		covered = 0;
		spans = textblock_spans(description, &n);
		check(n > 0, "ordinary item description retains semantic sections");
		for (i = 0; i < n; ++i) {
			check(spans[i].start == covered,
			      "item section boundaries preserve original prose");
			covered += spans[i].length;
		}
		check(covered == wcslen(textblock_text(description)),
		      "item sections cover complete description");
		textblock_free(description);
	}
}

static void map_contract(void)
{
	uint32_t saved[RAND_DEG], index = state_i, value = Rand_value;
	uint32_t saved_z[3] = {z0, z1, z2};
	bool quick = Rand_quick;
	int y, x, i;
	int *attrs = calloc(cave->mon_max, sizeof(*attrs));
	memcpy(saved, STATE, sizeof(saved));
	for (i = 1; i < cave->mon_max; ++i)
		attrs[i] = cave_monster(cave, i)->attr;
	for (y = 0; y < cave->height; ++y)
		for (x = 0; x < cave->width; ++x) {
			struct grid_data g;
			struct map_noise noise = {{4, L'!'}, {5, L'D'}};
			struct map_presentation a, b;
			int known = square(player->cave, loc(x, y))->feat;
			map_info_readonly(loc(x, y), &g);
			map_present(&g, &noise, &a);
			map_present(&g, &noise, &b);
			check(memcmp(&a, &b, sizeof(a)) == 0,
			      "map presentation is repeatable");
			check(square(player->cave, loc(x, y))->feat == known,
			      "map query does not memorize terrain");
		}
	{
		struct grid_data g;
		struct map_noise noise = {{4, L'!'}, {5, L'D'}};
		struct map_presentation result;
		map_info_readonly(player->grid, &g);
		g.hallucinate = true;
		g.is_player = false;
		g.m_idx = 1;
		map_present(&g, &noise, &result);
		check(result.top.attr == 5 && result.top.chr == L'D',
		      "hallucination uses caller's visual glyph");
	}
	{
		struct grid_data g = {0};
		struct trap trap = {0};
		struct map_noise noise = {{4, L'!'}, {5, L'D'}};
		struct map_presentation result;
		g.f_idx = FEAT_FLOOR;
		g.lighting = LIGHTING_LIT;
		g.first_kind = lookup_kind(TV_LIGHT, 1);
		g.is_player = true;
		trap.kind = &trap_info[1];
		trf_on(trap.flags, TRF_VISIBLE);
		g.trap = &trap;
		map_present(&g, &noise, &result);
		check(result.layers[MAP_TRAP].chr ==
		              trap_x_char[g.lighting][trap.kind->tidx],
		      "trap layer retained under actor and object");
		check(result.layers[MAP_OBJECT].chr == object_kind_char(g.first_kind),
		      "object layer retained under actor");
		check(result.layers[MAP_ACTOR].chr == result.top.chr,
		      "top glyph agrees with actor layer");
		g.is_player = false;
		trf_on(trap.flags, TRF_WEB);
		map_present(&g, &noise, &result);
		check(!result.layers[MAP_OBJECT].chr && !result.layers[MAP_ACTOR].chr &&
		              result.top.chr == result.layers[MAP_TRAP].chr,
		      "web occludes objects without inventing absent layers");
		g.hallucinate = true;
		g.m_idx = 1;
		map_present(&g, &noise, &result);
		check(result.layers[MAP_OBJECT].chr == L'!' &&
		              result.layers[MAP_ACTOR].chr == L'D',
		      "caller owns both hallucinated glyphs");
	}
	{
		struct grid_data g;
		int feature = square(player->cave, player->grid)->feat;
		player->cave->squares[player->grid.y][player->grid.x].feat = FEAT_NONE;
		map_info_readonly(player->grid, &g);
		check(g.f_idx == FEAT_NONE,
		      "query does not reveal unremembered terrain");
		player->cave->squares[player->grid.y][player->grid.x].feat = feature;
	}
	check(!memcmp(saved, STATE, sizeof(saved)) && index == state_i &&
	              value == Rand_value && quick == Rand_quick &&
	              saved_z[0] == z0 && saved_z[1] == z1 && saved_z[2] == z2,
	      "map queries leave complete gameplay RNG unchanged");
	for (i = 1; i < cave->mon_max; ++i)
		check(attrs[i] == cave_monster(cave, i)->attr,
		      "map queries leave monster colours unchanged");
	free(attrs);
}
static bool store_scenario;
static int store_gold;

static void fixture_enter_store(int feature)
{
	int y, x;
	for (y = 1; y < cave->height - 1; ++y)
		for (x = 1; x < cave->width - 1; ++x)
			if (square(cave, loc(x, y))->feat == feature) {
				monster_swap(player->grid, loc(x, y));
				handle_stuff(player);
				/* Settle delayed input flushes caused by fixture relocation. */
				inkey_scan = SCAN_INSTANT;
				(void)inkey_ex();
				return;
			}
	check(false, "fixture store exists");
}
static void select_store_item(cJSON *p, bool purchase)
{
	cJSON *entry, *items = cJSON_GetObjectItem(state, "items");
	cJSON_ArrayForEach(entry, items)
	{
		const char *where = text(entry, "location");
		if (purchase ? (!strcmp(where, "Store") || !strcmp(where, "Home"))
		             : (!strcmp(text(entry, "category"), "light") &&
		                strcmp(where, "Store") && strcmp(where, "Home"))) {
			cJSON_AddStringToObject(p, "item", text(entry, "id"));
			return;
		}
	}
	check(false, "native transaction item exists");
}
static const char *reply_store(cJSON *p, const char *type, cJSON *value)
{
	check(pending_prompt && !strcmp(text(pending_prompt, "type"), type),
	      "native store prompt type");
	cJSON_AddStringToObject(p, "prompt_id", text(pending_prompt, "prompt_id"));
	cJSON_AddItemToObject(p, "value", value);
	cJSON_Delete(pending_prompt);
	pending_prompt = NULL;
	return "prompt.reply";
}
static const char *store_step(cJSON *p)
{
	switch (step++) {
	case 3:
		document_contract();
		map_contract();
		fixture_object(TV_LIGHT, 1, 3);
		OPT(player, birth_no_selling) = false;
		player->au = 10000;
		fixture_enter_store(FEAT_STORE_GENERAL);
		{
			struct object *unknown = object_new();
			struct store *shop = store_at(cave, player->grid);
			int limit, owned, pack_size = z_info->pack_size;
			object_prep(
			        unknown,
			        lookup_kind(TV_WAND, lookup_sval(TV_WAND, "Fire Balls")), 0,
			        MINIMISE);
			unknown->known = object_new();
			object_set_base_known(player, unknown);
			check(!object_flavor_is_aware(unknown),
			      "capacity fixture is unidentified");
			z_info->pack_size = pack_slots_used(player);
			check(!store_purchase_limit(shop, unknown, &limit, &owned),
			      "shared capacity rule rejects unidentified purchase with "
			      "full pack");
			z_info->pack_size = pack_size;
			object_delete(NULL, NULL, &unknown->known);
			object_delete(NULL, NULL, &unknown);
			/* Fixture-only failure message was asserted through the return
			 * value. */
			msg_flag = false;
		}
		store_gold = player->au;
		cJSON_AddStringToObject(p, "command", "core.hold");
		return "command.execute";
	case 4:
	case 7:
		check(!strcmp(text(state, "phase"), "store"), "native store entered");
		check(player->au == store_gold,
		      "cancelled purchase leaves gold unchanged");
		select_store_item(p, true);
		return "store.buy";
	case 5:
	case 8:
	case 11:
	case 16:
	case 18:
		return reply_store(p, "quantity", cJSON_CreateNumber(1));
	case 6:
		return reply_store(p, "confirmation", cJSON_CreateBool(false));
	case 9:
	case 12:
		return reply_store(p, "confirmation", cJSON_CreateBool(true));
	case 10:
		check(player->au < store_gold, "native purchase spends gold");
		store_gold = player->au;
		select_store_item(p, false);
		return "store.sell";
	case 13:
		check(player->au > store_gold, "native sale receives gold");
		store_gold = player->au;
		return "store.leave";
	case 14:
		check(!strcmp(text(state, "readiness"), "ready"),
		      "store exit returns to game");
		fixture_enter_store(FEAT_HOME);
		cJSON_AddStringToObject(p, "command", "core.hold");
		return "command.execute";
	case 15:
		check(cJSON_IsTrue(cJSON_GetObjectItem(
		              cJSON_GetObjectItem(state, "store"), "home")),
		      "native home entered");
		select_store_item(p, false);
		return "store.sell";
	case 17:
		check(player->au == store_gold, "home stash does not change gold");
		check(store_at(cave, player->grid)->stock_num > 0,
		      "home received item");
		select_store_item(p, true);
		return "store.buy";
	case 19:
		check(player->au == store_gold, "home retrieve does not change gold");
		check(store_at(cave, player->grid)->stock_num == 0,
		      "home item retrieved");
		return "store.leave";
	case 20:
		check(!strcmp(text(state, "readiness"), "ready"),
		      "home exit returns to game");
		player->au = 0;
		fixture_enter_store(FEAT_STORE_GENERAL);
		cJSON_AddStringToObject(p, "command", "core.hold");
		return "command.execute";
	case 21:
		select_store_item(p, true);
		return "store.buy";
	case 22:
		check(player->au == 0 && !pending_prompt,
		      "unaffordable purchase rejected without a quantity prompt");
		check(cJSON_IsTrue(cJSON_GetObjectItem(
		              cJSON_GetObjectItem(state, "store"), "ready")),
		      "rejected purchase restores storefront");
		player->au = 10000;
		select_store_item(p, true);
		return "store.buy";
	case 23:
		return reply_store(p, "quantity", cJSON_CreateNull());
	case 24: {
		struct object *obj;
		check(player->au == 10000,
		      "quantity cancellation leaves gold unchanged");
		for (obj = player->gear; obj; obj = obj->next)
			if (obj->tval == TV_LIGHT)
				obj->note = quark_add("!d");
		select_store_item(p, false);
		return "store.sell";
	}
	case 25:
		return reply_store(p, "confirmation", cJSON_CreateBool(false));
	case 26:
		check(player->au == 10000 && !pending_prompt,
		      "inscription cancellation aborts transaction before quantity");
		return "store.leave";
	case 27:
		check(!strcmp(text(state, "readiness"), "ready"),
		      "failed and cancelled purchases leave session usable");
		return "session.close";
	default:
		check(false, "unexpected native store input");
	}
	return "";
}