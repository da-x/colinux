#ifndef __COLINUX_OS_WINNT_MONITOR_H__
#define __COLINUX_OS_WINNT_MONITOR_H__

#include <colinux/kernel/monitor.h>

#include "kernel/ddk.h"

typedef struct co_monitor_osdep {
	co_os_mutex_t mutex;
} co_monitor_osdep_t;

#endif
