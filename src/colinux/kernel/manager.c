/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <linux/kernel.h>

#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/mutex.h>

#include "manager.h"

co_rc_t co_manager_create_monitor(co_manager_t *manager, co_monitor_t **cmon_out)
{
	co_monitor_t *cmon;
	unsigned long pages;
	co_id_t id = 0;
	co_rc_t rc = CO_RC_OK;

	for (id=0; id < CO_MAX_MONITORS; id++)
		if (manager->monitors[id] == NULL)
			break;
		
	if (id >= CO_MAX_MONITORS) {
		co_debug("Warning: Out of coLinux'es\n");
		return CO_RC(ERROR);
	}

	co_debug("Allocated cmon%d\n", id);

	pages = (sizeof(*cmon) + PAGE_SIZE - 1)/PAGE_SIZE;
	cmon = co_os_alloc_pages(pages);
	if (!cmon) {
		rc = CO_RC(OUT_OF_MEMORY);
		goto out;
	}

	memset(cmon, 0, pages * PAGE_SIZE);
	cmon->manager = manager;
	cmon->state = CO_MONITOR_STATE_START;
	cmon->console_state = CO_MONITOR_CONSOLE_STATE_DETACHED;
	cmon->refcount = 1;
	cmon->id = id;
	
	rc = co_console_create(80, 25, 25, &cmon->console);
	if (!CO_OK(rc))
		goto out_free;

	rc = co_queue_init(&cmon->console_queue);
	if (!CO_OK(rc))
		goto out_free_console;

	rc = co_os_mutex_create(&cmon->console_mutex);
	if (!CO_OK(rc))
		goto out_free_console_queue;
	
	rc = co_monitor_os_init(cmon);
	if (!CO_OK(rc))
		goto out_free_console_mutex;

	manager->monitors[id] = cmon;

	*cmon_out = cmon;

	return rc;

out_free_console_mutex:
	co_os_mutex_destroy(cmon->console_mutex);

out_free_console_queue:
	co_queue_flush(&cmon->console_queue);

out_free_console:
	co_console_destroy(cmon->console);

out_free:
	co_os_free_pages(cmon, pages);

out:
	return rc;
}

void co_manager_destroy_monitor(co_monitor_t *cmon)
{
	unsigned long pages;
	co_id_t id;
	co_manager_t *manager;

	manager = cmon->manager;
	id = cmon->id;
	manager->monitors[id] = NULL;

	co_monitor_os_exit(cmon);

	co_os_mutex_destroy(cmon->console_mutex);
	if (cmon->console)
		co_console_destroy(cmon->console);

	pages = (sizeof(*cmon) + PAGE_SIZE - 1)/PAGE_SIZE;
	co_os_free_pages(cmon, pages);

	co_debug("Freed cmon%d\n", id);
}

co_rc_t co_manager_get_monitor(co_manager_t *manager, co_id_t id, co_monitor_t **cmon)
{
	if (id >= CO_MAX_MONITORS)
		return CO_RC(INVALID_CO_ID);

	*cmon = manager->monitors[id];
	if (*cmon == NULL) 
		return CO_RC(ERROR);

	(*cmon)->refcount++;

	return CO_RC(OK);
}

void co_manager_put_monitor(co_monitor_t *cmon)
{
	cmon->refcount--;

	if (cmon->refcount == 1) {
		if (cmon->state == CO_MONITOR_STATE_AWAITING_TERMINATION)
			co_monitor_os_exclusive(cmon);
	}

	if (cmon->refcount == 0)
		co_manager_destroy_monitor(cmon);
}
