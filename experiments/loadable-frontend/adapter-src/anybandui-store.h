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
	return (*frontend_engine_ptr->p_item_is_available)((struct object *)obj) &&
	       (*frontend_engine_ptr->p_store_will_buy_tester)(obj) &&
	       (!(*frontend_engine_ptr->p_object_is_equipped)((*frontend_engine_ptr->p_player)->body, obj) || (*frontend_engine_ptr->p_obj_can_takeoff)(obj));
}

static void anybandui_capture_store(cJSON *state, cJSON *items, int *index)
{
	struct object **stock =
	        (*frontend_engine_ptr->p_mem_zalloc)((*frontend_engine_ptr->p_z_info)->store_inven_max * sizeof(*stock));
	cJSON *shop = cJSON_CreateObject(), *stock_ids = cJSON_CreateArray();
	cJSON *inventory = cJSON_CreateArray(), *record;
	bool home = active_store->feat == FEAT_HOME;
	int i;
	string(shop, "name", (*frontend_engine_ptr->p_f_info)[active_store->feat].name);
	json_bool(shop, "home", home);
	json_bool(shop, "ready", !store_busy);
	json_bool(shop, "no_selling", OPT((*frontend_engine_ptr->p_player), birth_no_selling));
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
		if (!(*frontend_engine_ptr->p_item_is_available)(obj))
			continue;
		quote = cJSON_CreateObject();
		string(quote, "item_id", str(record, "id"));
		json_bool(quote, "eligible", anybandui_store_can_sell(obj));
		if (!home)
			number(quote, "unit_price", (*frontend_engine_ptr->p_price_item)(active_store, obj, true, 1));
		cJSON_AddItemToArray(inventory, quote);
	}
	(*frontend_engine_ptr->p_store_stock_list)(active_store, stock, (*frontend_engine_ptr->p_z_info)->store_inven_max);
	for (i = 0; i < active_store->stock_num; ++i) {
		cJSON *quote = cJSON_CreateObject(), *comparisons = cJSON_CreateArray();
		int slot = (*frontend_engine_ptr->p_wield_slot)(stock[i]), n;
		record = item_record(stock[i], home ? "Home" : "Store", ++*index);
		string(quote, "item_id", str(record, "id"));
		if (!home)
			number(quote, "unit_price",
			       (*frontend_engine_ptr->p_price_item)(active_store, stock[i], false, 1));
		/* Include every occupied matching slot (both rings, for instance),
		 * rather than inventing client-side equipment categories or bonus
		 * calculations. */
		if (slot >= 0 && slot < (*frontend_engine_ptr->p_player)->body.count) {
			for (n = 0; n < (*frontend_engine_ptr->p_player)->body.count; ++n) {
				struct object *equipped = (*frontend_engine_ptr->p_player)->body.slots[n].obj;
				size_t handle;
				if (!equipped ||
				    (*frontend_engine_ptr->p_player)->body.slots[n].type != (*frontend_engine_ptr->p_player)->body.slots[slot].type)
					continue;
				for (handle = 1; handle < item_handle_count; ++handle)
					if (item_handles[handle] == equipped) {
						char id[80];
						(*frontend_engine_ptr->p_strnfmt)(id, sizeof(id), "item-%lu-%u", revision,
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
	(*frontend_engine_ptr->p_mem_free)(stock);
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
	if (purchase ? !(*frontend_engine_ptr->p_pile_contains)(active_store->stock, obj)
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
	    !(*frontend_engine_ptr->p_get_item_allow)(
	            obj,
	            (*frontend_engine_ptr->p_cmd_lookup_key)(CMD_DROP, OPT((*frontend_engine_ptr->p_player), rogue_like_commands)
	                                             ? KEYMAP_MODE_ROGUE
	                                             : KEYMAP_MODE_ORIG),
	            CMD_DROP, false))
		return;
	if (purchase && !(*frontend_engine_ptr->p_store_purchase_limit)(active_store, obj, &maximum, &owned))
		return;
	if (purchase)
		(*frontend_engine_ptr->p_strnfmt)(prompt, sizeof(prompt), "%s how many%s? (max %d) ",
		        home ? "Take" : "Buy",
		        owned ? (*frontend_engine_ptr->p_format)(" (you have %d)", owned) : "", maximum);
	else
		(*frontend_engine_ptr->p_strnfmt)(prompt, sizeof(prompt), "%s how many? ",
		        home ? "Drop" : "Sell");
	amount = (*frontend_engine_ptr->p_get_quantity_for_item)(prompt, maximum, obj);
	if (amount <= 0 || closing)
		return;
	part = (*frontend_engine_ptr->p_object_new)();
	(*frontend_engine_ptr->p_object_copy_amt)(part, obj, amount);
	if (purchase ? !(*frontend_engine_ptr->p_inven_carry_okay)(part)
	             : !(*frontend_engine_ptr->p_store_check_num)(active_store, part)) {
		(*frontend_engine_ptr->p_msg)("%s", purchase ? "You cannot carry that many items."
		          : home   ? "Your home is full."
		                   : "I have not the room in my store to keep it.");
		(*frontend_engine_ptr->p_object_delete)(NULL, NULL, &part);
		return;
	}
	if (!home) {
		price = (*frontend_engine_ptr->p_price_item)(active_store, part, !purchase, amount);
		(*frontend_engine_ptr->p_object_desc)(name, sizeof(name), part,
		            ODESC_PREFIX | ODESC_FULL | (purchase ? ODESC_STORE : 0),
		            (*frontend_engine_ptr->p_player));
		(*frontend_engine_ptr->p_strnfmt)(prompt, sizeof(prompt), "%s %s?%s",
		        purchase                        ? "Buy"
		        : OPT((*frontend_engine_ptr->p_player), birth_no_selling) ? "Give"
		                                        : "Sell",
		        name,
		        purchase && (*frontend_engine_ptr->p_tval_is_book_k)(obj->kind) && !(*frontend_engine_ptr->p_obj_can_browse)(obj)
		                ? " (Can't use!)"
		                : "");
		if (!anybandui_store_check(prompt, price) || closing) {
			(*frontend_engine_ptr->p_object_delete)(NULL, NULL, &part);
			return;
		}
	}
	(*frontend_engine_ptr->p_object_delete)(NULL, NULL, &part);
	command = purchase ? (home ? CMD_RETRIEVE : CMD_BUY)
	                   : (home ? CMD_STASH : CMD_SELL);
	(*frontend_engine_ptr->p_cmdq_push)(command);
	(*frontend_engine_ptr->p_cmd_set_arg_item)((*frontend_engine_ptr->p_cmdq_peek)(), "item", obj);
	(*frontend_engine_ptr->p_cmd_set_arg_number)((*frontend_engine_ptr->p_cmdq_peek)(), "quantity", amount);
	(*frontend_engine_ptr->p_cmdq_pop)(CTX_STORE);
	(*frontend_engine_ptr->p_notice_stuff)((*frontend_engine_ptr->p_player));
	(*frontend_engine_ptr->p_handle_stuff)((*frontend_engine_ptr->p_player));
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
