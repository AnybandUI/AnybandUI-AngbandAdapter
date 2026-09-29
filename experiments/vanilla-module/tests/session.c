/* Runs the actual session and protocol in this process. No child processes,
 * pipe handles, desktop automation or access to another process's memory. */
#include "../src/session.h"
#include "cJSON.h"
#include "angband.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "obj-power.h"
#include "obj-tval.h"
static struct object *fixture_wand;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static cJSON *state, *pending_prompt, *classic, *initial_turn;
static unsigned step, serial, reads, checks, responses;
static bool closing_response, load_session, cancel_birth;
static cJSON *capabilities;
static const char *last_method;
static const char *text(const cJSON *o, const char *key)
{
	const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
	return cJSON_IsString(v) ? v->valuestring : "";
}
static void check(bool condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL step %u: %s\n", step, message);
		exit(1);
	}
	++checks;
}
static cJSON *params(void)
{
	cJSON *p = cJSON_CreateObject();
	cJSON_AddStringToObject(p, "session_id", "session-1");
	cJSON_AddStringToObject(p, "context", text(state, "context"));
	cJSON_AddStringToObject(p, "revision", text(state, "revision"));
	return p;
}
static struct object *fixture_object(int tval, int sval, int count)
{
	struct object *obj = object_new();
	object_prep(obj, lookup_kind(tval, sval), 0, MINIMISE);
	obj->number = count;
	obj->known = object_new();
	object_set_base_known(player, obj);
	object_touch(player, obj);
	inven_carry(player, obj, false, false);
	return obj;
}
static void select_wand(cJSON *p)
{
	cJSON *v;
	cJSON_ArrayForEach(v, cJSON_GetObjectItem(state, "items"))
	{
		if (!strcmp(text(v, "category"), "wand")) {
			cJSON_AddStringToObject(p, "item", text(v, "id"));
			return;
		}
	}
	check(false, "fixture wand is represented in the actual item snapshot");
}
static bool write_record(void *user, const char *line)
{
	cJSON *j = cJSON_Parse(line), *d;
	check(j != NULL, "valid protocol output");
	if (!strcmp(text(j, "kind"), "response")) {
		++responses;
		if (cJSON_GetObjectItem(j, "error")) {
			fprintf(stderr, "%s: %s\n", last_method, line);
			check(false, "unexpected protocol error");
		}
		if (!strcmp(last_method, "session.close") ||
		    !strcmp(last_method, "birth.cancel"))
			closing_response = true;
		if (!strcmp(last_method, "hello"))
			capabilities = cJSON_Duplicate(
			        cJSON_GetObjectItem(cJSON_GetObjectItem(j, "result"),
			                            "capabilities"),
			        true);
	} else if (!strcmp(text(j, "event"), "state.changed")) {
		d = cJSON_GetObjectItem(j, "data");
		cJSON_Delete(state);
		state = cJSON_Duplicate(d, true);
	} else if (!strcmp(text(j, "event"), "prompt.requested")) {
		cJSON_Delete(pending_prompt);
		pending_prompt = cJSON_Duplicate(cJSON_GetObjectItem(j, "data"), true);
	}
	cJSON_Delete(j);
	return true;
}
#include "contracts.h"
static bool read_record(void *user, char *line, size_t size)
{
	cJSON *j, *p = params(), *v;
	const char *method = NULL;
	char id[40], *encoded;
	check(++reads < 180, "bounded interaction count");
	/* Startup messages may require acknowledgement before normal play. */
	if ((step == 3 && strcmp(text(state, "readiness"), "ready")) ||
	    (step > 3 && !pending_prompt &&
	     cJSON_IsTrue(cJSON_GetObjectItem(state, "message_pending")))) {
		method = "terminal.input";
		cJSON_AddStringToObject(p, "key", "enter");
	} else if (store_scenario && step >= 3) {
		method = store_step(p);
	} else
		switch (step++) {
		case 0:
			method = "hello";
			cJSON_AddItemToObject(p, "protocols",
			                      cJSON_Parse("[{\"major\":1,\"minor\":0}]"));
			cJSON_AddNumberToObject(p, "max_frame_bytes", 4194304);
			cJSON_AddBoolToObject(p, "native_inventory", true);
			cJSON_AddBoolToObject(p, "native_equipment", true);
			break;
		case 1:
			method = load_session ? "session.load" : "session.new";
			if (load_session)
				step = 3;
			cJSON_AddStringToObject(p, "save", "DirectSession");
			cJSON_AddStringToObject(p, "race", "Human");
			cJSON_AddStringToObject(p, "class", "Warrior");
			cJSON_AddBoolToObject(p, "native_birth", true);
			break;
		case 2:
            if (getenv("ANYBANDUI_PRICE_CHECK")) {
                struct object *probe = object_new();
                bool found = false;
                int hit, damage;
                object_prep(probe, lookup_kind(TV_SWORD, lookup_sval(TV_SWORD, "Dagger")), 0, MINIMISE);
                for (hit = -20; hit <= 20 && !found; ++hit) {
                    for (damage = -20; damage <= 20 && !found; ++damage) {
                        probe->to_h = hit; probe->to_d = damage;
                        if (object_power(probe, false, NULL) == -1) {
                            fprintf(stderr, "Pricing regression: dagger (%d,%d), power -1\n", hit, damage);
                            fflush(stderr);
                            check(object_value_real(probe, 1) == 0, "negative-power price is zero without overflow");
                            found = true;
                        }
                    }
                }
                check(found, "found negative-one pricing fixture");
                object_delete(NULL, NULL, &probe);
            }
			check(!strcmp(text(state, "phase"), "birth"), "semantic birth");
			if (cancel_birth) {
				method = "birth.cancel";
				break;
			}
			method = "birth.action";
			cJSON_AddStringToObject(p, "action", "accept");
			cJSON_AddStringToObject(p, "name", "DirectSession");
			cJSON_AddStringToObject(p, "history",
			                        "In-process integration fixture.");
			break;
		case 3:
			if (load_session) {
				check(cJSON_GetObjectItem(state, "dungeon") != NULL,
				      "loaded semantic map");
				method = "session.close";
				step = 29;
				break;
			}
			/* Deterministic synthetic inventory, using the upstream test
			 * pattern. */
			fixture_object(TV_LIGHT, 1, 3);
			fixture_wand = fixture_object(
			        TV_WAND, lookup_sval(TV_WAND, "Fire Balls"), 1);
			check(!object_flavor_is_aware(fixture_wand),
			      "fixture begins unidentified");
			check(cJSON_GetObjectItem(state, "dungeon") != NULL,
			      "semantic dungeon");
			classic = cJSON_Duplicate(cJSON_GetObjectItem(state, "dungeon"),
			                          true);
			initial_turn =
			        cJSON_Duplicate(cJSON_GetObjectItem(state, "turn"), true);
			method = "dungeon.camera";
			cJSON_AddBoolToObject(p, "enabled", true);
			break;
		case 4: {
			cJSON *full = cJSON_GetObjectItem(state, "dungeon");
			cJSON *rows = cJSON_GetObjectItem(classic, "cells");
			int yy, xx, count = 0;
			int ox = cJSON_GetObjectItem(classic, "x")->valueint;
			int oy = cJSON_GetObjectItem(classic, "y")->valueint;
			check(cJSON_IsTrue(cJSON_GetObjectItem(full, "full_level")),
			      "full camera");
			for (yy = 0; yy < cJSON_GetArraySize(rows); ++yy) {
				cJSON *row = cJSON_GetArrayItem(rows, yy);
				for (xx = 0; xx < cJSON_GetArraySize(row); ++xx) {
					cJSON *cell = cJSON_GetArrayItem(row, xx), *other;
					if (!cJSON_GetArrayItem(cell, 10)->valueint)
						continue;
					other = cJSON_GetArrayItem(
					        cJSON_GetArrayItem(
					                cJSON_GetObjectItem(full, "cells"),
					                yy + oy),
					        xx + ox);
					check(cJSON_Compare(cell, other, true),
					      "observed/query layer parity");
					++count;
				}
			}
			check(count > 0, "nonempty map comparison");
			method = "dungeon.tiles";
			cJSON_AddNumberToObject(p, "id", 1);
			break;
		}
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			method = "dungeon.tiles";
			cJSON_AddNumberToObject(p, "id", step - 4);
			break;
		case 10:
			method = "dungeon.tiles";
			cJSON_AddNumberToObject(p, "id", 0);
			break;
		case 11:
			method = "dungeon.camera";
			cJSON_AddBoolToObject(p, "enabled", false);
			break;
		case 12:
			check(cJSON_Compare(initial_turn,
			                    cJSON_GetObjectItem(state, "turn"), true),
			      "presentation leaves turn unchanged");
			method = "targeting.begin";
			cJSON_AddStringToObject(p, "mode", "look");
			break;
		case 13:
			check(cJSON_GetObjectItem(state, "targeting") != NULL,
			      "direct targeting state");
			method = "targeting.control";
			cJSON_AddStringToObject(p, "operation", "cancel");
			break;
		case 14:
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.rest");
			break;
		case 15:
			check(pending_prompt &&
			              !strcmp(text(pending_prompt, "selection_kind"),
			                      "rest"),
			      "typed rest prompt");
			method = "prompt.reply";
			cJSON_AddStringToObject(p, "prompt_id",
			                        text(pending_prompt, "prompt_id"));
			cJSON_AddNullToObject(p, "value");
			cJSON_Delete(pending_prompt);
			pending_prompt = NULL;
			break;
		case 16:
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.drop");
			cJSON_ArrayForEach(v, cJSON_GetObjectItem(state, "items"))
			{
				cJSON *q = cJSON_GetObjectItem(v, "quantity");
				if (q && q->valueint > 1) {
					cJSON_AddStringToObject(p, "item", text(v, "id"));
					break;
				}
			}
			check(v != NULL, "stack available for quantity prompt");
			break;
		case 17:
			check(pending_prompt &&
			              !strcmp(text(pending_prompt, "type"), "quantity"),
			      "quantity prompt");
			check(cJSON_GetObjectItem(pending_prompt, "item") != NULL,
			      "quantity has explicit subject");
			method = "prompt.reply";
			cJSON_AddStringToObject(p, "prompt_id",
			                        text(pending_prompt, "prompt_id"));
			cJSON_AddNullToObject(p, "value");
			cJSON_Delete(pending_prompt);
			pending_prompt = NULL;
			break;
		case 18:
			check(!strcmp(text(state, "readiness"), "ready"),
			      "cancellation returns to ready");
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.use");
			select_wand(p);
			break;
		case 19:
			check(cJSON_IsTrue(cJSON_GetObjectItem(state, "aiming")),
			      "unidentified wand aiming");
			check(cJSON_GetObjectItem(state, "blast_radius")->valueint == 0,
			      "unknown effect is not disclosed");
			method = "terminal.input";
			cJSON_AddStringToObject(p, "key", "escape");
			break;
		case 20:
			object_flavor_aware(player, fixture_wand);
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.use");
			select_wand(p);
			break;
		case 21:
			check(cJSON_IsTrue(cJSON_GetObjectItem(state, "aiming")),
			      "known wand aiming");
			check(cJSON_GetObjectItem(state, "blast_radius")->valueint == 2,
			      "known effect comes from live command");
			method = "targeting.control";
			cJSON_AddStringToObject(p, "operation", "target");
			break;
		case 22:
			check(cJSON_GetObjectItem(state, "targeting") != NULL,
			      "nested targeting context");
			check(cJSON_GetObjectItem(state, "blast_radius")->valueint == 2,
			      "effect context survives nested targeting");
			method = "targeting.blast";
			cJSON_AddNumberToObject(
			        p, "x",
			        cJSON_GetObjectItem(cJSON_GetObjectItem(state, "player"),
			                            "x")
			                ->valueint);
			cJSON_AddNumberToObject(
			        p, "y",
			        cJSON_GetObjectItem(cJSON_GetObjectItem(state, "player"),
			                            "y")
			                ->valueint);
			break;
		case 23:
			method = "targeting.control";
			cJSON_AddStringToObject(p, "operation", "cancel");
			break;
		case 24:
			check(cJSON_IsTrue(cJSON_GetObjectItem(state, "aiming")),
			      "nested cancel returns to aim request");
			method = "terminal.input";
			cJSON_AddStringToObject(p, "key", "escape");
			break;
		case 25:
			check(!strcmp(text(state, "readiness"), "ready"),
			      "aim cancellation returns to command boundary");
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.throw");
			select_wand(p);
			break;
		case 26:
			check(cJSON_IsTrue(cJSON_GetObjectItem(state, "aiming")),
			      "throw aiming");
			check(cJSON_GetObjectItem(state, "blast_radius")->valueint == 0,
			      "known activation is not misattributed to throwing");
			method = "terminal.input";
			cJSON_AddStringToObject(p, "key", "escape");
			break;
		case 27:
			check(!strcmp(text(state, "readiness"), "ready"),
			      "throw cancellation restores readiness");
			method = "command.execute";
			cJSON_AddStringToObject(p, "command", "core.hold");
			break;
		case 28:
			check(cJSON_GetObjectItem(state, "turn")->valuedouble >
			              initial_turn->valuedouble,
			      "committed wait advances the engine");
			check(!strcmp(text(state, "readiness"), "ready"),
			      "turn returns to ready");
			method = "session.close";
			break;
		default:
			check(false, "unexpected extra input");
		}
	{
		const char *snapshot_path =
		        !strcmp(method, "session.close")
		                ? getenv("DIRECT_SESSION_SNAPSHOT")
		                : (store_scenario && step == 5
		                           ? getenv("DIRECT_SESSION_STORE_SNAPSHOT")
		                           : NULL);
		if (snapshot_path) {
			cJSON *fixture = cJSON_CreateObject();
			char *contents;
			FILE *file = fopen(snapshot_path, "wb");
			check(file != NULL, "open presentation fixture");
			cJSON_AddItemToObject(fixture, "state",
			                      cJSON_Duplicate(state, true));
			cJSON_AddItemToObject(fixture, "capabilities",
			                      cJSON_Duplicate(capabilities, true));
			contents = cJSON_PrintUnformatted(fixture);
			check(fputs(contents, file) >= 0 && fclose(file) == 0,
			      "write presentation fixture");
			cJSON_free(contents);
			cJSON_Delete(fixture);
		}
	}
	last_method = method;
	fprintf(stderr, "request %u: %s\n", step, method);
	fflush(stderr);
	j = cJSON_CreateObject();
	snprintf(id, sizeof(id), "native-%u", ++serial);
	cJSON_AddStringToObject(j, "kind", "request");
	cJSON_AddStringToObject(j, "id", id);
	cJSON_AddStringToObject(j, "method", method);
	cJSON_AddItemToObject(j, "params", p);
	encoded = cJSON_PrintUnformatted(j);
	check(strlen(encoded) + 2 < size, "request fits transport");
	snprintf(line, size, "%s\n", encoded);
	cJSON_free(encoded);
	cJSON_Delete(j);
	return true;
}
static bool poll_input(void *user)
{
	return false;
}
static void stop_session(void *user, int status)
{
	fprintf(stderr, "Unexpected fatal transport stop: %d\n", status);
	check(false, "normal lifecycle must return rather than terminate host");
}
int main(int argc, char **argv)
{
	const struct anybandui_io io = {NULL, read_record, write_record, poll_input,
	                                stop_session};
	load_session = getenv("DIRECT_SESSION_LOAD") != NULL;
	cancel_birth = getenv("DIRECT_SESSION_CANCEL_BIRTH") != NULL;
	store_scenario = getenv("DIRECT_SESSION_STORE") != NULL;
	check(anybandui_run(&io, argc, argv) == 0, "session returned successfully");
	check(closing_response && step == (cancel_birth     ? 3u
	                                   : store_scenario ? 28u
	                                                    : 29u),
	      "normal close returned to host");
	check(anybandui_run(&io, argc, argv) == 2,
	      "second session rejected safely");
	printf("PASS: session returned to host; %u checks, %u requests, %u "
	       "responses\n",
	       checks, serial, responses);
	return 0;
}
