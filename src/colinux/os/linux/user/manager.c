/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */ 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/manager.h>

#include "../ioctl.h"

co_manager_handle_t co_os_manager_open(void)
{
	int fd;

	fd = open("/proc/colinux/ioctl", O_RDWR);
	if (fd == -1)
		return NULL;

	return (co_manager_handle_t)(fd);
}

co_rc_t co_os_manager_ioctl(
	co_manager_handle_t kernel_device,
	unsigned long code,
	void *input_buffer,
	unsigned long input_buffer_size,
	void *output_buffer,
	unsigned long output_buffer_size,
	unsigned long *output_returned)
{
	int fd = (int)kernel_device, ret;
	co_linux_io_t lio;

	lio.code = code;
	lio.input_buffer = input_buffer;
	lio.input_buffer_size = input_buffer_size;
	lio.output_buffer = output_buffer;
	lio.output_buffer_size = output_buffer_size;
	lio.output_returned = output_returned;

	ret = ioctl(fd, CO_LINUX_IOCTL_ID, &lio);
	if (ret != 0)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

void co_os_manager_close(co_manager_handle_t handle)
{
	close((int)(handle));
}

co_rc_t co_os_manager_is_installed(bool_t *installed)
{
	struct stat buf;
	int ret;

	ret = stat("/proc/colinux", &buf);
	if (installed)
		*installed = (ret != -1);

	return CO_RC(OK);
}
