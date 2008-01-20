/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <colinux/kernel/scsi.h>

int scsi_file_open(co_monitor_t *cmon, co_scsi_dev_t *dp)
{
	return CO_RC(ERROR);
}

int scsi_file_close(co_monitor_t *cmon, co_scsi_dev_t *dp)
{
	return CO_RC(ERROR);
}

int scsi_file_io(co_monitor_t *cmon, co_scsi_dev_t *dp, co_scsi_io_t *iop)
{
	return CO_RC(ERROR);
}

int scsi_file_size(co_monitor_t *cmon, co_scsi_dev_t *dp, unsigned long long *size)
{
	return CO_RC(ERROR);
}

int scsi_pass(co_monitor_t *cmon, co_scsi_dev_t *dp, co_scsi_pass_t *pass)
{
	return CO_RC(ERROR);
}
