#include "angband.h"
#include "store.h"
#include "obj-gear.h"
#include "obj-util.h"
#include "obj-knowledge.h"
#include "message.h"
bool store_purchase_limit(struct store *store, const struct object *obj,
	int *maximum, int *owned)
{
	int amt, num;
	int32_t price;
	bool flavor_aware;

	if (store->feat == FEAT_HOME) {
		amt = obj->number;
	} else {
		/* Price of one */
		price = price_item(store, obj, false, 1);

		/* Check if the player can afford any at all */
		if ((uint32_t)player->au < (uint32_t)price) {
			msg("You do not have enough gold for this item.");
			return false;
		}

		/* Work out how many the player can afford */
		if (price == 0)
			amt = obj->number; /* Prevent division by zero */
		else
			amt = player->au / price;

		if (amt > obj->number) amt = obj->number;

		/* Double check for wands/staves */
		if ((player->au >= price_item(store, obj, false, amt+1)) &&
			(amt < obj->number))
			amt++;
	}

	/* Limit to the number that can be carried */
	amt = MIN(amt, inven_carry_num(player, obj));

	/* Fail if there is no room.  Don't leak information about
	 * unknown flavors for a purchase (getting it from home doesn't
	 * leak information since it doesn't show the true flavor). */
	flavor_aware = object_flavor_is_aware(obj);
	if (amt <= 0 || (!flavor_aware && store->feat != FEAT_HOME &&
			pack_is_full())) {
		msg("You cannot carry that many items.");
		return false;
	}

	/* Find the number of this item in the inventory.  As above,
	 * avoid leaking information about unknown flavors. */
	if (!flavor_aware && store->feat != FEAT_HOME)
		num = 0;
	else
		num = find_inven(obj);
	*maximum = amt;
	*owned = num;
	return true;
}



