
#ifndef __COLINUX_KERNEL_SCSI_H__
#define __COLINUX_KERNEL_SCSI_H__

#include <colinux/common/config.h>
#include <colinux/common/scsi_types.h>
#include <scsi/coscsi.h>

#include "monitor.h"

struct co_scsi_dev;
typedef struct co_scsi_dev co_scsi_dev_t;

/* Driver */
struct co_scsi_dev {
	co_monitor_t *mp;
	co_scsi_dev_desc_t *conf;
	void *os_handle;
};
//} PACKED_STRUCT;

extern co_rc_t co_monitor_scsi_dev_init(co_scsi_dev_t *dev, int unit, co_scsi_dev_desc_t *conf);
extern void co_monitor_scsi_register_device(struct co_monitor *cmon, unsigned int unit, co_scsi_dev_t *dev);
extern void co_monitor_unregister_and_free_scsi_devices(struct co_monitor *cmon);
extern void co_scsi_op(co_monitor_t *);

/* O/S interface */
extern int scsi_file_open(co_scsi_dev_t *);
extern int scsi_file_close(co_scsi_dev_t *);
extern int scsi_file_io(co_scsi_dev_t *, co_scsi_io_t *);
extern int scsi_file_size(co_scsi_dev_t *,unsigned long long *);

/* Additional Sense Code (ASC) */
#define NO_ADDITIONAL_SENSE 0x0
#define LOGICAL_UNIT_NOT_READY 0x4
#define UNRECOVERED_READ_ERR 0x11
#define PARAMETER_LIST_LENGTH_ERR 0x1a
#define DATA_TRANSFER_ERROR 0x1b
#define INVALID_OPCODE 0x20
#define ADDR_OUT_OF_RANGE 0x21
#define INVALID_FIELD_IN_CDB 0x24
#define INVALID_FIELD_IN_PARAM_LIST 0x26
#define POWERON_RESET 0x29
#define SAVING_PARAMS_UNSUP 0x39
#define MEDIUM_NOT_PRESENT 0x3a
#define TRANSPORT_PROBLEM 0x4b
#define THRESHOLD_EXCEEDED 0x5d
#define LOW_POWER_COND_ON 0x5e


#endif
