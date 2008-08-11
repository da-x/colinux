/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_WINNT_KERNEL_TIME_H__
#define __CO_WINNT_KERNEL_TIME_H__

#include <colinux/kernel/monitor.h>

#include "ddk.h"

unsigned long windows_time_to_unix_time(LARGE_INTEGER time);
LARGE_INTEGER unix_time_to_windows_time(unsigned long time);

#endif
