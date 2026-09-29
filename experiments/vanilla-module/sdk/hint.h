#ifndef HINT_H
#define HINT_H

/*
 * A hint.
 */
struct hint {
	char *hint;
	struct hint *next;
};

extern __declspec(dllimport) struct hint *hints; /* store.c */

#endif /* HINT_H */


