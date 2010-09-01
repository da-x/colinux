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

#include <colinux/common/config.h>

struct co_block_dev;
struct co_monitor;

typedef struct co_block_dev co_block_dev_t;

struct co_block_dev {
	unsigned long long size;
	co_block_dev_desc_t *conf;
	co_rc_t (*service)(struct co_monitor *cmon, co_block_dev_t *dev,
			   co_block_request_t *request);
	void (*free)(struct co_monitor *cmon, co_block_dev_t *dev);
	unsigned int use_count;
	int unit;
} PACKED_STRUCT;

extern void co_monitor_block_register_device(struct co_monitor *cmon, unsigned int unit,
					     co_block_dev_t *dev);
extern void co_monitor_block_request(struct co_monitor *cmon, unsigned int unit,
					co_block_request_t *request);
extern void co_monitor_block_unregister_device(struct co_monitor *cmon, unsigned int unit);
extern void co_monitor_unregister_and_free_block_devices(struct co_monitor *cmon);

#endif
