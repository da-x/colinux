/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "manager.h"

#include <colinux/os/alloc.h>

co_rc_t co_os_get_pfn_ptr(struct co_manager *manager, co_pfn_t pfn, co_os_mdl_ptr_t **mdl_out)
{
	co_os_mdl_ptr_t *current = &manager->osdep->map_root;
	int level = MDL_MAPPING_LEVELS;

	while (level > 0) {
		unsigned long index;

		if (current->more_mdls == NULL) {
			current->more_mdls = co_os_malloc(sizeof(co_os_mdl_ptr_t)*MDL_MAPPING_ENTRIES);
			if (current->more_mdls == NULL)
				return CO_RC(ERROR);
			manager->osdep->blocks_allocated++;

			memset(current->more_mdls, 0, sizeof(co_os_mdl_ptr_t)*MDL_MAPPING_ENTRIES);
		}

		index = ((pfn >> ((level-1)*MDL_MAPPING_ENTRIES_SCALE)) & (MDL_MAPPING_ENTRIES - 1));
		current = &current->more_mdls[index];
		level -= 1;
	}
	
	*mdl_out = current;

	return CO_RC(OK);
}

void co_os_free_pfn_tracker(co_os_mdl_ptr_t *current, int level, unsigned long *pages, unsigned long *lists)
{
	int i;

	if (level == 0) {
		switch (current->type) {
		case CO_OS_MDL_PTR_TYPE_MDL:
			if (current->mdl != NULL) {
				(*pages)++;
				MmFreePagesFromMdl(current->mdl);
				IoFreeMdl(current->mdl);
			}
			break;
		case CO_OS_MDL_PTR_TYPE_PAGE:
			if (current->page != NULL) {
				(*pages)++;
				co_os_free_pages(current->page, 1);
			}
			break;
		default:
			break;
		}
		return;
	}

	if (current->more_mdls == NULL)
		return;

	for (i=0; i < MDL_MAPPING_ENTRIES; i++)
		co_os_free_pfn_tracker(&current->more_mdls[i], level-1, pages, lists);

	co_os_free(current->more_mdls);	
	(*lists)++;
}

co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep)
{
	co_rc_t rc = CO_RC(OK);

	*osdep = (typeof(*osdep))(co_os_malloc(sizeof(**osdep)));
	if (*osdep == NULL)
		return CO_RC(ERROR);

	memset(*osdep, 0, sizeof(**osdep));

	return rc;
}

void co_os_manager_free(co_osdep_manager_t osdep)
{
	unsigned long pages = 0;
	unsigned long lists = 0;

	co_os_free_pfn_tracker(&osdep->map_root, MDL_MAPPING_LEVELS, &pages, &lists);

	co_debug("manager: peak allocation: %d mdls, %d aux\n", osdep->mdls_peak_allocation, osdep->auxiliary_peak_allocation);
	co_debug("manager: freed: %d pages, %d lists\n", pages, lists);

	co_os_free(osdep);
}

