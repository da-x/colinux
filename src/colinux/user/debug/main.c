/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

#include <colinux/os/alloc.h>
#include <colinux/user/manager.h>

#define BUFFER_SIZE   (0x100000)

int co_debug_main(int argc, char *argv[])
{
	co_manager_handle_t handle;
	co_rc_t rc;
	int ret = 0;

	handle = co_os_manager_open();
	if (handle) {
		char *buffer = (char *)co_os_malloc(BUFFER_SIZE);
		if (buffer) {
			co_manager_ioctl_debug_reader_t debug_reader;
			debug_reader.user_buffer = buffer;
			debug_reader.user_buffer_size = BUFFER_SIZE;
			while (1) {
				debug_reader.filled = 0;
				rc = co_manager_debug_reader(handle, &debug_reader);
				if (!CO_OK(rc)) {
					fprintf(stderr, "log ended: %x\n", rc);
					return rc;
				}
				write(1, buffer, debug_reader.filled);
			}
		}
		co_os_free(buffer);
	}

	co_os_manager_close(handle);

	return ret;
}
