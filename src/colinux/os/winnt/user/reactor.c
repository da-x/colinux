#include <windows.h>

#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/reactor.h>

#include "reactor.h"

co_rc_t co_os_reactor_select(co_reactor_t handle, int miliseconds)
{
	HANDLE wait_list[handle->num_users*2];
	co_reactor_user_t users[handle->num_users*2];
	co_reactor_user_t user;
	int index = 0, count = 0;
	co_rc_t rc = CO_RC(OK);
	ULONG wait_time;
	ULONG status;

	co_list_each_entry(user, &handle->users, node) {
		if (user->os_data->read_enabled) {
			wait_list[index] = user->os_data->read_event;
			users[index] = user;
			index += 1;
		}
		if (user->os_data->write_enabled) {
			wait_list[index] = user->os_data->write_event;
			users[index] = user;
			index += 1;
		}
	}

	count = index;

	wait_time = INFINITE;
	if (miliseconds != -1)
		wait_time = miliseconds;

	status = WaitForMultipleObjects(count, wait_list, FALSE, wait_time);

	if (status >= WAIT_OBJECT_0  &&  status < WAIT_OBJECT_0 + count) {
		index = status - WAIT_OBJECT_0;
		user = users[index];

		if (user->os_data->read_enabled) {
			if (user->os_data->read_event == wait_list[index]) {
				rc = user->last_read_rc = user->os_data->read(user);
				return rc;
			}
		}
		if (user->os_data->write_enabled) {
			if (user->os_data->write_event == wait_list[index]) {
				user->os_data->write(user);
			}
		}
	}

	return rc;
}

static co_rc_t packet_read_async(co_winnt_reactor_packet_user_t handle)
{
	BOOL result;
	DWORD error;

	result = ReadFile(handle->rhandle,
			  &handle->buffer,
			  sizeof(handle->buffer),
			  &handle->size,
			  &handle->read_overlapped);

	if (!result) {
		error = GetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
			return CO_RC(OK);
		case ERROR_NOT_ENOUGH_MEMORY:
			/* Hack for serial daemon */
			co_debug_error("Warn: NOT_ENOUGH_MEMORY (handle=%d)", (int)handle->rhandle);
			return CO_RC(OUT_OF_MEMORY);
		default:
			co_debug_error("Error: 0x%lx", error);
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}

static co_rc_t packet_read_completed(co_winnt_reactor_packet_user_t handle)
{
	BOOL result;

	result = GetOverlappedResult(handle->rhandle,
				     &handle->read_overlapped,
				     &handle->size, FALSE);

	if (result) {
		handle->user.received(&handle->user, handle->buffer, handle->size);
		return packet_read_async(handle);
	} else {
		if (GetLastError() == ERROR_BROKEN_PIPE) {
			co_debug_error("Pipe broken, exiting");
			return CO_RC(BROKEN_PIPE);
		}

		co_debug_error("GetOverlappedResult error 0x%lx", GetLastError());
	}

	return CO_RC(OK);
}

static co_rc_t packet_send_whole(co_winnt_reactor_packet_user_t handle,
				 unsigned char *buffer, unsigned long size)
{
	BOOL result;
	DWORD error;
	unsigned long write_size;

	result = WriteFile(handle->whandle, buffer, size,
			   &write_size, &handle->write_overlapped);

	/* check write_size == size */

	if (!result) {
		switch (error = GetLastError())
		{
		case ERROR_IO_PENDING:
			WaitForSingleObject(handle->os_user.write_event, INFINITE);
			break;
		default:
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}

static co_rc_t packet_read(co_reactor_user_t user)
{
	return packet_read_completed((co_winnt_reactor_packet_user_t)user);
}

static co_rc_t packet_send(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	return packet_send_whole((co_winnt_reactor_packet_user_t)user, buffer, size);
}

static void packet_write(co_reactor_user_t user)
{
}

co_rc_t co_winnt_reactor_packet_user_create(
	co_reactor_t reactor, HANDLE whandle, HANDLE rhandle,
	co_reactor_user_receive_func_t receive,
	co_winnt_reactor_packet_user_t *handle_out)
{
	co_winnt_reactor_packet_user_t user;
	co_rc_t rc;

	user = co_os_malloc(sizeof(*user));
	if (!user)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(user, 0, sizeof(*user));

	user->whandle = whandle;
	user->rhandle = rhandle;

	user->os_user.read_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	user->os_user.write_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	user->os_user.read = packet_read;
	user->os_user.write = packet_write;
	user->os_user.read_enabled = PTRUE;

	user->read_overlapped.Offset = 0;
	user->read_overlapped.OffsetHigh = 0;
	user->read_overlapped.hEvent = user->os_user.read_event;

	user->write_overlapped.Offset = 0;
	user->write_overlapped.OffsetHigh = 0;
	user->write_overlapped.hEvent = user->os_user.write_event;

	user->user.os_data = &user->os_user;
	user->user.reactor = reactor;
	user->user.received = receive;
	user->user.send = packet_send;

	co_reactor_add(reactor, &user->user);

	rc = packet_read_async(user);
	if (!CO_OK(rc)) {
		// Hack for coserial daemon (kbhit and getch)
		// Ignore error from first ReadFile
		if (CO_RC_GET_CODE(rc) != CO_RC_OUT_OF_MEMORY ||
		    rhandle != GetStdHandle(STD_INPUT_HANDLE)) {
			co_winnt_reactor_packet_user_destroy(user);
			return rc;
		}
	}

	*handle_out = user;

	return CO_RC(OK);
}

void co_winnt_reactor_packet_user_destroy(co_winnt_reactor_packet_user_t user)
{
	CloseHandle(user->os_user.read_event);
	CloseHandle(user->os_user.write_event);
	co_reactor_remove(&user->user);
	co_os_free(user);
}

