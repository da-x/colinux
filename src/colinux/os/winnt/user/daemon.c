/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */ 

#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "daemon.h"
#include <colinux/os/alloc.h>

co_rc_t co_os_open_daemon_pipe(co_id_t linux_id, co_module_t module_id, co_daemon_handle_t *handle_out)
{
	HANDLE handle;
	long written = 0;
	char pathname[0x100];
	BOOL ret;
	co_daemon_handle_t daemon_handle;
	int timeout = 10;

	snprintf(pathname, sizeof(pathname), "\\\\.\\pipe\\coLinux%d", (int)linux_id);

	co_debug("pipe client %d/%d: Connecting to daemon...\n", linux_id, module_id);
	
	ret = WaitNamedPipe(pathname, NMPWAIT_USE_DEFAULT_WAIT);
	if (!ret) { 
		co_debug("Connection timed out (%x)\n", GetLastError());
		return CO_RC(ERROR);
	}

	co_debug("pipe client %d/%d: Connection established\n", linux_id, module_id);

	daemon_handle = co_os_malloc(sizeof(*daemon_handle));
	if (!daemon_handle)
		return CO_RC(ERROR);

	handle = CreateFile (
		pathname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0
		);

	/* Identify to the daemon */
	ret = WriteFile(handle, &module_id, sizeof(module_id), &written, NULL); 
	if (!ret) {
		co_debug("pipe client %d/%d: Attachment failed\n", linux_id, module_id);
		CloseHandle(handle);
		co_os_free(daemon_handle);
		return CO_RC(ERROR);
	}

	daemon_handle->handle = handle;

	*handle_out = daemon_handle;

	return CO_RC(OK);
}

co_rc_t co_os_daemon_peek_messages(co_daemon_handle_t handle, bool_t *available)
{
	DWORD BytesLeftThisMessage = 0;
	BOOL ret;
	
	ret = PeekNamedPipe(handle->handle,
			    NULL, 0, NULL, NULL,
			    &BytesLeftThisMessage);

	*available = BytesLeftThisMessage != 0;

	return CO_RC(OK);
}

co_rc_t co_os_daemon_get_message(co_daemon_handle_t handle, co_message_t **message)
{
	DWORD BytesLeftThisMessage = 0;
	BOOL ret;
	char *buf;
	unsigned long bytes_read = 0;
	
	ret = PeekNamedPipe(handle->handle,
			    NULL, 0, NULL, NULL,
			    &BytesLeftThisMessage);

	if (BytesLeftThisMessage != 0) {
		buf = (char *)malloc(BytesLeftThisMessage);
		if (!buf)
			return CO_RC(ERROR);

		ret = ReadFile(handle->handle, buf, BytesLeftThisMessage, &bytes_read, NULL);
		if (ret  &&  (BytesLeftThisMessage == bytes_read)) {
			*message = (co_message_t *)buf;
			return CO_RC(OK);
		}

		co_os_free(buf);
	}

	message = NULL;

	return CO_RC(ERROR);
}

co_rc_t co_os_daemon_send_message(co_daemon_handle_t handle, co_message_t *message)
{
	unsigned long bytes_written;
	unsigned long bytes_to_write = sizeof(*message) + message->size;
	BOOL ret;

	ret = WriteFile(handle->handle, (char *)message, 
			bytes_to_write,	&bytes_written, NULL);

	if (ret != TRUE)
		return CO_RC(ERROR);

	if (bytes_written != bytes_written)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

void co_os_daemon_close(co_daemon_handle_t handle)
{
	CloseHandle(handle->handle);
	co_os_free(handle);
}

