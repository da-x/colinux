#ifndef __COLINUX_OS_WINNT_MONITOR_H__
#define __COLINUX_OS_WINNT_MONITOR_H__

#include <colinux/kernel/monitor.h>

#include "kernel/ddk.h"

typedef struct co_monitor_osdep {
	co_os_mutex_t 	mutex;

	/* ligong liu, support kernel mode conet */
	co_os_mutex_t 	conet_mutex;
	char 		protocol_name[128];
	void*		conet_protocol;
	co_list_t 	conet_adapters; /* lists of conet_adapter */
} co_monitor_osdep_t;

#endif
