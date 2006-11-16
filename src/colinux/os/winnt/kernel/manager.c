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

co_rc_t co_os_manager_init(co_manager_t *manager, co_osdep_manager_t *osdep)
{
	co_rc_t rc = CO_RC(OK);
	int i;

	*osdep = (typeof(*osdep))(co_os_malloc(sizeof(**osdep)));
	if (*osdep == NULL)
		return CO_RC(ERROR);

	co_global_manager = manager;

	memset(*osdep, 0, sizeof(**osdep));

	co_list_init(&(*osdep)->mdl_list);
	co_list_init(&(*osdep)->mapped_allocated_list);
	co_list_init(&(*osdep)->pages_unused);
	for (i=0; i < PFN_HASH_SIZE; i++)
		co_list_init(&(*osdep)->pages_hash[i]);

	MmResetDriverPaging(&co_global_manager);
	
	rc = co_os_mutex_create(&(*osdep)->mutex);

	return rc;
}

void co_os_manager_free(co_osdep_manager_t osdep)
{
	co_debug("before free: %d mdsl, %d pages\n", osdep->mdls_allocated, osdep->pages_allocated);
	
	co_winnt_free_all_pages(osdep);

	co_os_mutex_destroy(osdep->mutex);
	
	co_debug("after free: %d mdsl, %d pages\n", osdep->mdls_allocated, osdep->pages_allocated);
	
	co_os_free(osdep);
}
