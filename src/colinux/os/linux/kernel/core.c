/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <colinux/common/debug.h>
#include <colinux/kernel/monitor.h>
#include <colinux/os/kernel/alloc.h>

COLINUX_DEFINE_MODULE("colinux-driver");

co_rc_t co_monitor_os_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK;

	cmon->osdep = NULL;

	return rc;
}

void co_monitor_os_exit(co_monitor_t *cmon)
{
}

