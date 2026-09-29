/** \file frontend-module.h
 * \brief Entry contract for build-matched native frontends.
 *
 * Released under the same licences as Angband.
 */
#ifndef INCLUDED_FRONTEND_MODULE_H
#define INCLUDED_FRONTEND_MODULE_H

struct frontend_module {
	const char *build_id;
	int (*run)(int argc, char **argv);
};

/* Returns false when the ordinary frontend was requested. */
__declspec(dllimport) int frontend_module_start(int *status);

#endif


