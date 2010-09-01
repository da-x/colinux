/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_FILE_BLOCK_H__
#define __COLINUX_KERNEL_FILE_BLOCK_H__

#include "block.h"

typedef enum {
	CO_MONITOR_FILE_BLOCK_CLOSED,
	CO_MONITOR_FILE_BLOCK_OPENED,
} co_monitor_file_block_state_t;

struct co_monitor_file_block_dev;
typedef struct co_monitor_file_block_dev co_monitor_file_block_dev_t;

struct co_monitor;

typedef struct {
	co_rc_t (*get_size)(co_monitor_file_block_dev_t *fdev, unsigned long long *size);
	co_rc_t (*open)(struct co_monitor *cmon, co_monitor_file_block_dev_t *fdev);
	co_rc_t (*read)(struct co_monitor *cmon, co_block_dev_t *dev,
			co_monitor_file_block_dev_t *fdev, co_block_request_t *request);
	co_rc_t (*write)(struct co_monitor *cmon, co_block_dev_t *dev,
			 co_monitor_file_block_dev_t *fdev, co_block_request_t *request);
	co_rc_t (*close)(co_monitor_file_block_dev_t *fdev);
} co_monitor_file_block_operations_t;

struct co_monitor_file_block_dev {
	co_block_dev_t dev; /* Must stay as the first field */

	co_monitor_file_block_state_t state;
	co_pathname_t pathname;
	co_monitor_file_block_operations_t *op;

	struct co_os_file_block_sysdep *sysdep;
};

co_rc_t co_monitor_file_block_init(struct co_monitor *cmon, co_monitor_file_block_dev_t *dev, co_pathname_t *pathname);
void co_monitor_file_block_shutdown(co_monitor_file_block_dev_t *dev);

extern co_monitor_file_block_operations_t co_os_file_block_async_operations;
extern co_monitor_file_block_operations_t co_os_file_block_default_operations;

#endif
