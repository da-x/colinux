
#ifndef __COLINUX_KERNEL_SCSI_H__
#define __COLINUX_KERNEL_SCSI_H__

#include <colinux/common/config.h>
#include <colinux/common/scsi_types.h>

#include "monitor.h"

/* Mode/Inq page data */
struct scsi_page {
	int num;
	unsigned char *page;
	int size;
};
typedef struct scsi_page scsi_page_t;

struct scsi_rom {
	char *name;			/* Device name */
	int *commands;			/* commands */
	scsi_page_t *inq;
	scsi_page_t *mode;
};
typedef struct scsi_rom scsi_rom_t;

struct co_scsi_dev;
typedef struct co_scsi_dev co_scsi_dev_t;

/* Driver */
struct co_scsi_dev {
	co_monitor_t *mp;
	co_scsi_dev_desc_t *conf;
	scsi_rom_t *rom;
	unsigned long flags;
	unsigned long long size;
	unsigned long block_size;
	unsigned long long max_lba;
	int bs_bits;
	void *os_handle;
	int key;
	int asc;
	int asq;
};
//} PACKED_STRUCT;

/* Flags */
#define SCSI_DF_RMB	1		/* Removeable */

typedef struct {
	co_scsi_dev_t *dp;
	co_monitor_t *mp;
	co_scsi_request_t req;
	unsigned char msg[512];
} scsi_worker_t;

extern co_rc_t co_monitor_scsi_dev_init(co_scsi_dev_t *dev, int unit, co_scsi_dev_desc_t *conf);
extern void co_monitor_scsi_register_device(struct co_monitor *cmon, unsigned int unit, co_scsi_dev_t *dev);
extern co_rc_t co_monitor_scsi_request(struct co_monitor *cmon, unsigned int unit, co_scsi_request_t *srp);
extern void co_monitor_scsi_unregister_device(struct co_monitor *cmon, unsigned int unit);
extern void co_monitor_unregister_and_free_scsi_devices(struct co_monitor *cmon);

/* O/S interface */
extern int scsi_file_open(co_scsi_dev_t *);
extern int scsi_file_close(co_scsi_dev_t *);
extern int scsi_file_rw(scsi_worker_t *,unsigned long long, unsigned long, int);
extern void scsi_queue_worker(scsi_worker_t *,void (*func)(scsi_worker_t *));

/* Open flags */
#define SCSI_FILE_SEQ 1		/* Sequential operation */

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
