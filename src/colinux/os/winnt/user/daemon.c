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
	co_rc_t rc = CO_RC(OK);

	daemon_handle = co_os_malloc(sizeof(*daemon_handle));
	if (!daemon_handle) {
		rc = CO_RC(ERROR);
		goto out_error;
	}

	bzero(daemon_handle, sizeof(*daemon_handle));

	daemon_handle->read_buffer = co_os_malloc(CO_DAEMON_PIPE_BUFFER_SIZE);
	if (!daemon_handle->read_buffer) {
		rc = CO_RC(ERROR);
		goto out_free;
	}
	
	daemon_handle->read_overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (daemon_handle->read_overlap.hEvent == NULL) {
		rc = CO_RC(ERROR);
		goto out_free_buffer;
	}

	snprintf(pathname, sizeof(pathname), "\\\\.\\pipe\\coLinux%d", (int)linux_id);

	co_debug("pipe client %d/%d: Connecting to daemon...\n", linux_id, module_id);
	
	ret = WaitNamedPipe(pathname, NMPWAIT_USE_DEFAULT_WAIT);
	if (!ret) { 
		co_debug("Connection timed out (%x)\n", GetLastError());
		rc = CO_RC(ERROR);
		goto out_close_event;
	}

	co_debug("pipe client %d/%d: Connection established\n", linux_id, module_id);

	handle = CreateFile (
		pathname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		0
		);

	/* Identify ourselves to the daemon */
	ret = WriteFile(handle, &module_id, sizeof(module_id), &written, NULL); 
	if (!ret) {
		co_debug("pipe client %d/%d: Attachment failed\n", linux_id, module_id);
		goto out_close_file;
	}

	daemon_handle->handle = handle;

	*handle_out = daemon_handle;

	return CO_RC(OK);

/* Error path */
out_close_file:
	CloseHandle(handle);

out_close_event:
	CloseHandle(daemon_handle->read_overlap.hEvent);

out_free_buffer:
	co_os_free(daemon_handle->read_buffer);

out_free:
	co_os_free(daemon_handle);

out_error:
	return rc;
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

co_rc_t co_os_daemon_read(co_daemon_handle_t handle)
{
	BOOL result;

	ResetEvent(handle->read_overlap.hEvent);

	result = ReadFile(handle->handle,
			  handle->read_buffer,
			  CO_DAEMON_PIPE_BUFFER_SIZE,
			  &handle->read_size,
			  &handle->read_overlap);

	if (!result) { 
		DWORD error = GetLastError();
		
		switch (error)
		{ 
		case ERROR_IO_PENDING: 
			handle->read_pending = PTRUE;
			return CO_RC(OK);

		case ERROR_BROKEN_PIPE:
			handle->read_pending = PFALSE;
			return CO_RC(BROKEN_PIPE);

		default:
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}

co_rc_t co_os_daemon_read_complete(co_daemon_handle_t handle,
				   unsigned long timeout)
{
	BOOL result;
	HANDLE pipe_handle[1] = {handle->read_overlap.hEvent, };

	result = MsgWaitForMultipleObjects(1,
					   pipe_handle,
					   FALSE,
					   timeout,
					   QS_ALLEVENTS);

	if (result == WAIT_TIMEOUT)
		return CO_RC(TIMEOUT);

	if (result == WAIT_OBJECT_0 + 1) {
		/* 
		 * Window messages are pending, fake a timeout and
		 * let the process handle the messsages.
		 */
		return CO_RC(TIMEOUT);
	}

	handle->read_pending = PFALSE;

	result = GetOverlappedResult(
		handle->handle,
		&handle->read_overlap,
		&handle->read_size,
		FALSE);

	if (!result) { 
		DWORD error = GetLastError();
		
		if (error == ERROR_BROKEN_PIPE)
			return CO_RC(BROKEN_PIPE);

		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t co_os_daemon_copy_message(co_daemon_handle_t handle,
				  co_message_t **message_out)
{
	co_message_t *message;
	char *buf = NULL;
	unsigned long message_size;
	unsigned long buffer_left;

	*message_out = NULL;

	buffer_left = handle->read_ptr_end - handle->read_ptr;
	if (buffer_left == 0)
		return CO_RC(OK);

	message = (co_message_t *)handle->read_ptr;
	message_size = (sizeof(*message) + message->size);

	if (buffer_left < message_size) {
		co_debug("partial message received (%d < %d)\n", 
			 buffer_left, message_size);
		return CO_RC(ERROR);
	}
	
	buf = (char *)malloc(message_size);
	if (!buf) {
		handle->read_ptr += message_size;
		return CO_RC(ERROR);
	}

	memcpy(buf, handle->read_ptr, message_size);
	*message_out = (co_message_t *)buf;
	handle->read_ptr += message_size;

	return CO_RC(OK);
}

co_rc_t co_os_daemon_get_message(co_daemon_handle_t handle, 
				 co_message_t **message_out,
				 unsigned long timeout)
{
	co_rc_t rc = CO_RC(OK);

	*message_out = NULL;

	rc = co_os_daemon_copy_message(handle, message_out);
	if (!CO_OK(rc))
		return rc;

	if (*message_out != NULL)
		return CO_RC(OK);

	if (!handle->read_pending) {
		rc = co_os_daemon_read(handle);
		if (!CO_OK(rc)) {
			return rc;
		}
	}

	if (handle->read_pending) {
		rc = co_os_daemon_read_complete(handle, timeout); 
		if (!CO_OK(rc)) {
			return rc;
		}
	}

	handle->read_ptr = handle->read_buffer;
	handle->read_ptr_end = handle->read_buffer + handle->read_size;

	return co_os_daemon_copy_message(handle, message_out);
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
	if (handle->read_pending)
		CancelIo(handle->handle);

	CloseHandle(handle->handle);
	CloseHandle(handle->read_overlap.hEvent);
	co_os_free(handle->read_buffer);
	co_os_free(handle);
}
