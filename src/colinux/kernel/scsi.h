
#ifndef __COLINUX_KERNEL_SCSI_H__
#define __COLINUX_KERNEL_SCSI_H__

/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

#include <colinux/common/config.h>
#include <scsi/coscsi.h>

#include "monitor.h"

struct co_scsi_dev;
typedef struct co_scsi_dev co_scsi_dev_t;

/* Driver */
struct co_scsi_dev {
	int unit;
	co_scsi_dev_desc_t *conf;
	void *os_handle;
};

extern co_rc_t co_monitor_scsi_dev_init(co_scsi_dev_t *dev, int unit, co_scsi_dev_desc_t *conf);
extern void co_monitor_scsi_register_device(struct co_monitor *cmon, unsigned int unit, co_scsi_dev_t *dev);
extern void co_monitor_unregister_and_free_scsi_devices(struct co_monitor *cmon);
extern void co_scsi_request(co_monitor_t *, int op, int unit);

/* O/S interface */
extern int scsi_file_open(co_monitor_t *, co_scsi_dev_t *);
extern int scsi_file_close(co_monitor_t *, co_scsi_dev_t *);
extern int scsi_file_io(co_monitor_t *, co_scsi_dev_t *, co_scsi_io_t *);
extern int scsi_file_size(co_monitor_t *, co_scsi_dev_t *,unsigned long long *);
extern int scsi_pass(co_monitor_t *, co_scsi_dev_t *, co_scsi_pass_t *);

#endif
