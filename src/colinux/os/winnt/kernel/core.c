/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "ddk.h"

#include <colinux/kernel/monitor.h>
#include <colinux/kernel/network.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/current/monitor.h>

co_rc_t co_monitor_os_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK;
	
	cmon->osdep = (co_monitor_osdep_t *)co_os_malloc(sizeof(*cmon->osdep));
	if (cmon->osdep == NULL) {
		return CO_RC(OUT_OF_MEMORY);
	}

	KeInitializeEvent(&cmon->osdep->console_event, SynchronizationEvent, FALSE);
	KeInitializeEvent(&cmon->osdep->network_event, SynchronizationEvent, FALSE);
	KeInitializeEvent(&cmon->osdep->interrupt_event, SynchronizationEvent, FALSE);

	KeInitializeEvent(&cmon->osdep->exclusiveness, NotificationEvent, FALSE);
	KeInitializeEvent(&cmon->osdep->debugstop, SynchronizationEvent, FALSE);
	KeInitializeEvent(&cmon->osdep->console_detach, SynchronizationEvent, FALSE);

	return rc;
}

void co_monitor_os_exit(co_monitor_t *cmon)
{
	co_os_free(cmon->osdep);
}

void co_monitor_os_cancel_userspace(co_monitor_t *cmon)
{
}

void co_monitor_os_network_poll(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->network_event, UserRequest, 
			      KernelMode, FALSE, NULL);
}

void co_monitor_os_network_event(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->network_event, 1, FALSE);
}

void co_monitor_os_console_poll(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->console_event, UserRequest, 
			      KernelMode, FALSE, NULL);
}

void co_monitor_os_console_event(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->console_event, 1, FALSE);
}

void co_monitor_os_idle(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->interrupt_event, UserRequest, 
			      KernelMode, FALSE, NULL);
}

void co_monitor_os_wakeup(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->interrupt_event, 1, FALSE);
}

#if (0)
unsigned long co_monitor_os_sysdep_ioctl(co_monitor_t *cmon, co_sysdep_ioctl_t *params)
{
	switch (params->op) {
	case CO_OS_SYSDEP_SET_NETDEV: {
		DbgPrint("Network device used: %s\n", params->set_netdev.pathname);
		break;
	}
	}

	return 0;
}

#endif

void co_monitor_os_exclusive(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->exclusiveness, 1, FALSE);
}

void co_monitor_os_clear_console_detach_event(co_monitor_t *cmon)
{
	KeClearEvent(&cmon->osdep->console_detach);
}

void co_monitor_os_set_console_detach_event(co_monitor_t *cmon)
{
	KeSetEvent(&cmon->osdep->console_detach, 1, FALSE);
}

void co_monitor_os_wait_console_detach(co_monitor_t *cmon)
{
	KeWaitForSingleObject(&cmon->osdep->console_detach,
			      UserRequest, KernelMode,
			      FALSE, NULL);
}
