/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "ddk.h"

#include <colinux/kernel/monitor.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/current/monitor.h>

COLINUX_DEFINE_MODULE("colinux-driver");

co_rc_t co_monitor_os_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK;

	cmon->osdep = co_os_malloc(sizeof(*cmon->osdep));
	if (cmon->osdep == NULL) {
		return CO_RC(OUT_OF_MEMORY);
	}

	co_os_mutex_create(&cmon->osdep->mutex);
	return rc;
}

void co_monitor_os_exit(co_monitor_t *cmon)
{
	co_os_mutex_destroy(cmon->osdep->mutex);
	co_os_free(cmon->osdep);
}
