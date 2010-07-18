
/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"

#include <colinux/kernel/transfer.h>
#include <colinux/kernel/fileblock.h>

typedef struct {
	loff_t offset;
	co_monitor_file_block_dev_t *fdev;
} co_os_transfer_file_block_data_t;

static
co_rc_t co_os_transfer_file_block(struct co_monitor *cmon,
				  void *host_data, void *linuxvm, unsigned long size,
				  co_monitor_transfer_dir_t dir)
{
	co_os_transfer_file_block_data_t *data;
	co_rc_t rc = CO_RC_OK;
	struct file *filp;

	data = (co_os_transfer_file_block_data_t *)host_data;
	filp = (struct file *)(data->fdev->sysdep);

	if (CO_MONITOR_TRANSFER_FROM_HOST == dir) {
		mm_segment_t fs;
		size_t size_read;

		if (!filp->f_op->read)
			return CO_RC(ERROR);

		fs = get_fs();
		set_fs(KERNEL_DS);
		size_read = filp->f_op->read(filp, linuxvm, size, &data->offset);
		set_fs(fs);

		if (size_read != size) {
			co_debug("co_os_transfer_file_block: read error: %d != %ld",
				 size_read, size);
			rc = CO_RC(ERROR);
		}
	}
	else {
		mm_segment_t fs;
		size_t size_written;

		if (!filp->f_op->write)
			return CO_RC(ERROR);

		fs = get_fs();
		set_fs(KERNEL_DS);
		size_written = filp->f_op->write(filp, linuxvm, size, &data->offset);
		set_fs(fs);

		if (size_written != size) {
			co_debug("co_os_transfer_file_block: write error: %d != %ld",
				 size_written, size);
			rc = CO_RC(ERROR);
		}
	}

	return rc;
}

static
co_rc_t co_os_file_block_read(struct co_monitor *linuxvm,
			      co_block_dev_t *dev,
			      co_monitor_file_block_dev_t *fdev,
			      co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;

	data.offset = request->offset;
	data.fdev = fdev;

	rc = co_monitor_host_linuxvm_transfer(linuxvm,
					      &data,
					      co_os_transfer_file_block,
					      request->address,
					      (unsigned long)request->size,
					      CO_MONITOR_TRANSFER_FROM_HOST);

	return rc;
}


static
co_rc_t co_os_file_block_write(struct co_monitor *linuxvm,
			       co_block_dev_t *dev,
			       co_monitor_file_block_dev_t *fdev,
			       co_block_request_t *request)
{
	co_rc_t rc;
	co_os_transfer_file_block_data_t data;

	data.offset = request->offset;
	data.fdev = fdev;

	rc = co_monitor_host_linuxvm_transfer(linuxvm,
					      &data,
					      co_os_transfer_file_block,
					      request->address,
					      (unsigned long)request->size,
					      CO_MONITOR_TRANSFER_FROM_LINUX);
	return rc;
}

static
co_rc_t co_os_file_block_get_size(co_monitor_file_block_dev_t *fdev, unsigned long long *size)
{
	struct file *filp;
	struct inode *inode = NULL;
	co_rc_t rc = CO_RC(OK);

	filp = filp_open(fdev->pathname, O_RDONLY | O_LARGEFILE, 0);
        if (IS_ERR(filp)) {
		co_debug("error opening file '%s' (errno %ld)",
		         (fdev->pathname) ? fdev->pathname : "(NULL)", PTR_ERR(filp));
		return CO_RC(ERROR);
	}

        if (filp->f_dentry)
                inode = filp->f_dentry->d_inode;

	if (inode) {
		loff_t inode_size;
		inode_size = i_size_read(inode);
		*size = inode_size;
	}
	else
		rc = CO_RC(ERROR);

	filp_close(filp, current->files);

	return rc;
}

static
co_rc_t co_os_file_block_open(struct co_monitor *linuxvm, co_monitor_file_block_dev_t *fdev)
{
	struct file *filp;
	struct inode *inode = NULL;
	co_rc_t rc = CO_RC(OK);

	co_debug("opening %s", fdev->pathname);

	filp = filp_open(fdev->pathname, O_RDWR | O_LARGEFILE, 0);
        if (IS_ERR(filp))
		return CO_RC(ERROR);

        if (filp->f_dentry)
                inode = filp->f_dentry->d_inode;

        if (inode && S_ISBLK(inode->i_mode)) {
                if (bdev_read_only(inode->i_bdev)) {
			/* TODO... */
		}
        } else if (!inode || !S_ISREG(inode->i_mode)) {
                co_debug("colinux: invalid file type: %s", fdev->pathname);
		rc = CO_RC(ERROR);
		goto out;
        }

	fdev->sysdep = (struct co_os_file_block_sysdep *)(filp);
	return rc;

out:
	filp_close(filp, current->files);
	return rc;
}

static
co_rc_t co_os_file_block_close(co_monitor_file_block_dev_t *fdev)
{
	struct file *filp;

	co_debug("closing %s", fdev->pathname);

	filp = (struct file *)(fdev->sysdep);
	filp_close(filp, NULL);

	return CO_RC(OK);
}

co_monitor_file_block_operations_t co_os_file_block_async_operations = {
	.open = co_os_file_block_open,
	.close = co_os_file_block_close,
	.read = co_os_file_block_read,
	.write = co_os_file_block_write,
	.get_size = co_os_file_block_get_size,
};

co_monitor_file_block_operations_t co_os_file_block_default_operations = {
	.open = co_os_file_block_open,
	.close = co_os_file_block_close,
	.read = co_os_file_block_read,
	.write = co_os_file_block_write,
	.get_size = co_os_file_block_get_size,
};
