/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_BLOCK_H__
#define __COLINUX_KERNEL_BLOCK_H__

#include "monitor.h"

struct co_block_dev;
typedef struct co_block_dev co_block_dev_t;

struct co_block_dev {
	unsigned long long size;
	co_rc_t (*service)(co_monitor_t *cmon, co_block_dev_t *dev, 
			   co_block_request_t *request);
	void (*free)(co_monitor_t *cmon, co_block_dev_t *dev);
	long use_count;
} PACKED_STRUCT;

extern void co_monitor_block_register_device(co_monitor_t *cmon, unsigned long index, 
					     co_block_dev_t *dev);
extern co_rc_t co_monitor_block_request(co_monitor_t *cmon, unsigned long index, 
					co_block_request_t *request);
extern void co_monitor_block_unregister_device(co_monitor_t *cmon, unsigned long index);
extern void co_monitor_unregister_and_free_block_devices(co_monitor_t *cmon);

#endif
