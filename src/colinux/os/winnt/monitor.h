#ifndef __COLINUX_OS_WINNT_MONITOR_H__
#define __COLINUX_OS_WINNT_MONITOR_H__

#include <colinux/kernel/monitor.h>

#include "kernel/ddk.h"

typedef struct co_monitor_osdep {
	KEVENT exclusiveness;
	KEVENT debugstop;

	KEVENT interrupt_event;
	KEVENT network_event;
	KEVENT console_event;

	KEVENT console_detach;
} co_monitor_osdep_t;

#endif
