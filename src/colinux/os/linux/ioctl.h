/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_LINUX_IOCTL_H__
#define __CO_LINUX_IOCTL_H__

typedef struct co_linux_io {
	unsigned long 	code;
	void*		input_buffer;
	unsigned long	input_buffer_size;
	void*		output_buffer;
	unsigned long	output_buffer_size;
	unsigned long*	output_returned;
} co_linux_io_t;

#define CO_LINUX_IOCTL_ID  0x12340000

#endif
