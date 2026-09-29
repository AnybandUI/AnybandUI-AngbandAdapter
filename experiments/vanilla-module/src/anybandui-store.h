/* Semantic storefront adapter. Included by main-anybandui.c so engine handles
 * remain private to the adapter. No pricing or transaction rules live here. */
static struct store *active_store;
static bool store_busy;
static enum { STORE_IDLE, STORE_BUY, STORE_SELL, STORE_LEAVE } store_operation;
static struct object *store_selection;
static void publish(void);
static void pump(void);

static bool anybandui_store_can_sell(const struct object *obj)
{
	return item_is_available((struct object *)obj) &&
	       store_will_buy_tester(obj) &&
	       (!object_is_equipped(player->body, obj) || obj_can_takeoff(obj));
}

static void anybandui_capture_store(cJSON *state, cJSON *items, int *index)
{
	struct object **stock =
	        mem_zalloc(z_info->store_inven_max * sizeof(*stock));
	cJSON *shop = cJSON_CreateObject(), *stock_ids = cJSON_CreateArray();
	cJSON *inventory = cJSON_CreateArray(), *record;
	bool home = active_store->feat == FEAT_HOME;
	int i;
	string(shop, "name", f_info[active_store->feat].name);
	json_bool(shop, "home", home);
	json_bool(shop, "ready", !store_busy);
	json_bool(shop, "no_selling", OPT(player, birth_no_selling));
	if (!home && active_store->owner) {
		string(shop, "owner", active_store->owner->name);
		number(shop, "owner_purse", active_store->owner->max_cost);
	}
	/* Inventory quotes and eligibility come from the current engine store. */
	i = 0;
	cJSON_ArrayForEach(record, items)
	{
		struct object *obj = item_handles[++i];
		cJSON *quote;
		if (!item_is_available(obj))
			continue;
		quote = cJSON_CreateObject();
		string(quote, "item_id", str(record, "id"));
		json_bool(quote, "eligible", anybandui_store_can_sell(obj));
		if (!home)
			number(quote, "unit_price", price_item(active_store, obj, true, 1));
		cJSON_AddItemToArray(inventory, quote);
	}
	store_stock_list(active_store, stock, z_info->store_inven_max);
	for (i = 0; i < active_store->stock_num; ++i) {
		cJSON *quote = cJSON_CreateObject(), *comparisons = cJSON_CreateArray();
		int slot = wield_slot(stock[i]), n;
		record = item_record(stock[i], home ? "Home" : "Store", ++*index);
		string(quote, "item_id", str(record, "id"));
		if (!home)
			number(quote, "unit_price",
			       price_item(active_store, stock[i], false, 1));
		/* Include every occupied matching slot (both rings, for instance),
		 * rather than inventing client-side equipment categories or bonus
		 * calculations. */
		if (slot >= 0 && slot < player->body.count) {
			for (n = 0; n < player->body.count; ++n) {
				struct object *equipped = player->body.slots[n].obj;
				size_t handle;
				if (!equipped ||
				    player->body.slots[n].type != player->body.slots[slot].type)
					continue;
				for (handle = 1; handle < item_handle_count; ++handle)
					if (item_handles[handle] == equipped) {
						char id[80];
						strnfmt(id, sizeof(id), "item-%lu-%u", revision,
						        (unsigned)handle);
						cJSON_AddItemToArray(comparisons,
						                     cJSON_CreateString(id));
						break;
					}
			}
		}
		cJSON_AddItemToObject(quote, "compare_with", comparisons);
		cJSON_AddItemToArray(items, record);
		cJSON_AddItemToArray(stock_ids, quote);
	}
	mem_free(stock);
	cJSON_AddItemToObject(shop, "stock", stock_ids);
	cJSON_AddItemToObject(shop, "inventory", inventory);
	cJSON_AddItemToObject(state, "store", shop);
}

static void anybandui_store_request(const char *id, const char *method,
                                    cJSON *params)
{
	cJSON *record;
	struct object *obj = NULL;
	int i = 0;
	bool purchase = streq(method, "store.buy");
	if (!streq(str(params, "context"), context_text)) {
		error(id, "stale_revision", "Store contents changed; choose again.");
		return;
	}
	if (!active_store || store_busy || active_prompt ||
	    store_operation != STORE_IDLE) {
		error(id, "busy", "Finish the current interaction first.");
		return;
	}
	if (streq(method, "store.leave")) {
		store_operation = STORE_LEAVE;
		response(id, cJSON_CreateObject());
		return;
	}
	cJSON_ArrayForEach(record, cJSON_GetObjectItem(snapshot, "items"))
	{
		++i;
		if (streq(str(record, "id"), str(params, "item")) &&
		    i < (int)item_handle_count)
			obj = item_handles[i];
	}
	if (!obj) {
		error(id, "stale_handle", "Select a current item.");
		return;
	}
	if (purchase ? !pile_contains(active_store->stock, obj)
	             : !anybandui_store_can_sell(obj)) {
		error(id, "invalid_argument",
		      "That item is not available for this transaction.");
		return;
	}
	store_selection = obj;
	store_operation = purchase ? STORE_BUY : STORE_SELL;
	response(id, cJSON_CreateObject());
}

/* Select and confirm directly, then let Angband execute the transaction. */
static void anybandui_store_transaction(bool purchase)
{
	struct object *obj = store_selection, *part;
	bool home = active_store->feat == FEAT_HOME;
	int maximum = obj->number, owned = 0, amount;
	int32_t price;
	char prompt[240], name[160];
	cmd_code command;

	/* Preserve the terminal selector's inscription confirmations. */
	if (!purchase &&
	    !get_item_allow(
	            obj,
	            cmd_lookup_key(CMD_DROP, OPT(player, rogue_like_commands)
	                                             ? KEYMAP_MODE_ROGUE
	                                             : KEYMAP_MODE_ORIG),
	            CMD_DROP, false))
		return;
	if (purchase && !store_purchase_limit(active_store, obj, &maximum, &owned))
		return;
	if (purchase)
		strnfmt(prompt, sizeof(prompt), "%s how many%s? (max %d) ",
		        home ? "Take" : "Buy",
		        owned ? format(" (you have %d)", owned) : "", maximum);
	else
		strnfmt(prompt, sizeof(prompt), "%s how many? ",
		        home ? "Drop" : "Sell");
	amount = get_quantity_for_item(prompt, maximum, obj);
	if (amount <= 0 || closing)
		return;
	part = object_new();
	object_copy_amt(part, obj, amount);
	if (purchase ? !inven_carry_okay(part)
	             : !store_check_num(active_store, part)) {
		msg("%s", purchase ? "You cannot carry that many items."
		          : home   ? "Your home is full."
		                   : "I have not the room in my store to keep it.");
		object_delete(NULL, NULL, &part);
		return;
	}
	if (!home) {
		price = price_item(active_store, part, !purchase, amount);
		object_desc(name, sizeof(name), part,
		            ODESC_PREFIX | ODESC_FULL | (purchase ? ODESC_STORE : 0),
		            player);
		strnfmt(prompt, sizeof(prompt), "%s %s?%s",
		        purchase                        ? "Buy"
		        : OPT(player, birth_no_selling) ? "Give"
		                                        : "Sell",
		        name,
		        purchase && tval_is_book_k(obj->kind) && !obj_can_browse(obj)
		                ? " (Can't use!)"
		                : "");
		if (!anybandui_store_check(prompt, price) || closing) {
			object_delete(NULL, NULL, &part);
			return;
		}
	}
	object_delete(NULL, NULL, &part);
	command = purchase ? (home ? CMD_RETRIEVE : CMD_BUY)
	                   : (home ? CMD_STASH : CMD_SELL);
	cmdq_push(command);
	cmd_set_arg_item(cmdq_peek(), "item", obj);
	cmd_set_arg_number(cmdq_peek(), "quantity", amount);
	cmdq_pop(CTX_STORE);
	notice_stuff(player);
	handle_stuff(player);
}

static void anybandui_store_session(struct store *store)
{
	active_store = store;
	ready = false;
	phase = "store";
	store_busy = false;
	store_operation = STORE_IDLE;
	for (;;) {
		publish();
		while (store_operation == STORE_IDLE && !closing)
			pump();
		if (store_operation == STORE_LEAVE || closing)
			break;
		store_busy = true;
		anybandui_store_transaction(store_operation == STORE_BUY);
		store_selection = NULL;
		store_operation = STORE_IDLE;
		store_busy = false;
	}
	active_store = NULL;
	store_operation = STORE_IDLE;
}

