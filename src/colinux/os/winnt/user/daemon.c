/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <windows.h>
#include <stdio.h>

#include "daemon.h"

#include <colinux/os/alloc.h>
#include <colinux/common/messages.h>

/*
 * Queue iteration threshold for packet retention and deallocation
 */
 
#define PACKET_RETENTION 3

typedef struct _packet {
	struct _packet *next;
	unsigned size;
	unsigned free;			// ==0 allocated, !=0 iteration count
	co_message_t message[0];
} packet_t;

static packet_t *
packet_allocate(co_daemon_handle_t daemon, size_t size)
{
	packet_t *packet = daemon->packets;
	packet_t **shadow = (packet_t**)&daemon->packets;

	while(packet) {
		if(packet->free) {
			if(packet->size >= size && size >= (packet->size >> 1)) {
				packet->free = 0;
				*shadow = packet->next;
				packet->next = 0;
				return packet;
			}
			if(++packet->free <= PACKET_RETENTION) {
				shadow = &packet->next;
				packet = packet->next;
				continue;
			}
			*shadow = packet->next;
			HeapFree(daemon->heap, HEAP_NO_SERIALIZE, packet);
			packet = *shadow;
			continue;
		}
		shadow = &packet->next;
		packet = packet->next;
	}

	if(!(packet = HeapAlloc(daemon->heap, HEAP_NO_SERIALIZE, sizeof(packet_t)+size)))
		return NULL;
	packet->next = 0;
	packet->free = 0;
	packet->size = size;
	return packet;
}

static DWORD WINAPI io_thread(LPVOID param)
{
	co_daemon_handle_t daemon = param;
	HANDLE pipe = daemon->handle;
	OVERLAPPED overlapped;
	bool_t async = PFALSE;
	packet_t *packet = 0;
	HANDLE events[2];
	packet_t *p = 0;
	co_rc_t rc;
	DWORD size;
	DWORD code;

#	define NT_ERROR(_1) { \
		co_debug_lvl(pipe,1,"co_os_daemon_thread() error [ %x , %s ]\n", \
			GetLastError(), \
			_1); \
		goto co_os_daemon_thread_error; \
	}
	
	daemon->qIn = daemon->qOut = daemon->packets = 0;
	co_debug_lvl(pipe,10,"user daemon thread started");
	daemon->heap = HeapCreate(HEAP_NO_SERIALIZE, 0x10000, 0x100000);

	memset(&overlapped, 0, sizeof (overlapped));
	overlapped.hEvent = CreateEvent(0, TRUE, FALSE, 0);

	if (!overlapped.hEvent)
		NT_ERROR("CE001");

	events[0] = overlapped.hEvent;
	events[1] = daemon->shifted;

      co_os_daemon_thread_loop:
	if (WaitForSingleObject(events[1], INFINITE) == WAIT_FAILED)
		NT_ERROR("W0001");

	while (daemon->loop) {
		if ((!daemon->message) && daemon->qOut) {
			p = daemon->qOut;
			if(! (daemon->qOut = p->next) )
				daemon->qIn = 0;
			p->next = daemon->packets;
			daemon->packets = p;
			daemon->message = p->message;
			// p = 0;
			if (!SetEvent(daemon->readable))
				NT_ERROR("SE0001");
		}
		if (async) {
			code = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if(code == WAIT_FAILED)
				NT_ERROR("W0002");
			if (!daemon->message)
				if (!ResetEvent(daemon->readable))
					NT_ERROR("RE0001");
			if (code != WAIT_OBJECT_0)
				continue;
			if (!GetOverlappedResult(pipe, &overlapped, &size, FALSE)) {
				code = GetLastError();
				if (code == ERROR_IO_INCOMPLETE)
					continue;
				if (code == ERROR_IO_PENDING)
					continue;
				if (code == ERROR_BROKEN_PIPE)
					goto co_os_daemon_thread_broken_pipe;
				NT_ERROR("GOR01");
			}
			if(!size)	{
				packet->free = 1;
				packet->next = daemon->packets;
				daemon->packets = packet;
				}
			else	{
				if(size < sizeof(co_message_t))
					NT_ERROR("DS0001");
				if(size != packet->message->size + sizeof(co_message_t))
					NT_ERROR("DS0002");
				if(!daemon->qOut) {
					daemon->qOut = daemon->qIn = packet;
				} else {
					((packet_t*)daemon->qIn)->next = packet;
					daemon->qIn = packet;
				}
			}
			// packet = 0;
			async = PFALSE;
		}
		if (!async) {
			if (!PeekNamedPipe(pipe, 0, 0, 0, 0, &size))
				NT_ERROR("PNP01");
			if (!(packet = packet_allocate(daemon,size)))
				goto co_os_daemon_thread_loop;
			async = PTRUE;
			if (!ReadFile(pipe, packet->message, size, 0, &overlapped)) {
				code = GetLastError();
				if (code == ERROR_IO_INCOMPLETE)
					continue;
				if (code == ERROR_IO_PENDING)
					continue;
				if (code == ERROR_BROKEN_PIPE)
					goto co_os_daemon_thread_broken_pipe;
				NT_ERROR("RF001");
			}
		}
	}

	rc = CO_RC(OK);

	co_os_daemon_thread_return:
	if (async) {
		CancelIo(pipe);
		HeapFree(daemon->heap, HEAP_NO_SERIALIZE, packet);
		async = 0;
	}
	while (daemon->loop && daemon->qOut) {
		if (!daemon->message) {
			p = daemon->qOut;
			if(! (daemon->qOut = p->next) )
				daemon->qIn = 0;
			p->next = daemon->packets;
			daemon->packets = p;
			daemon->message = p->message;
			// p = 0;
			if (!SetEvent(daemon->readable)) {
				if (!CO_OK(rc))
					break;
				NT_ERROR("SE0002");
			}
		}
		if (WaitForSingleObject(events[1], INFINITE) == WAIT_FAILED) {
			if (!CO_OK(rc))
				break;
			NT_ERROR("W0003");
		}
		if (!daemon->message)
			if (!ResetEvent(daemon->readable)) {
				if (!CO_OK(rc))
					break;
				NT_ERROR("RE0002");
			}
	}
	if (overlapped.hEvent)
		if ((!CloseHandle(overlapped.hEvent)) && CO_OK(rc))
			co_debug_lvl(pipe,1,"co_os_daemon_thread() error"
				 " [ %x , CH0001 ]\n", GetLastError());
	daemon->rc = rc;
	if (daemon->loop)
		if ((!SetEvent(daemon->readable)) && CO_OK(rc))
			co_debug_lvl(pipe,1,"co_os_daemon_thread() error"
				 " [ %x , SE0003 ]\n", GetLastError());

	return 0;

      co_os_daemon_thread_error:
	rc = CO_RC(ERROR);
	goto co_os_daemon_thread_return;

      co_os_daemon_thread_broken_pipe:
	rc = CO_RC(BROKEN_PIPE);
	goto co_os_daemon_thread_return;
}

co_rc_t
co_os_daemon_message_deallocate(co_daemon_handle_t daemon, co_message_t *message)
{
	packet_t *packet = (packet_t*)
		((unsigned)message - sizeof(packet_t));
	packet->free = 1;
	return CO_RC(OK);
}

co_rc_t
co_os_daemon_message_receive(co_daemon_handle_t daemon,
			 co_message_t ** message_out, unsigned long timeout)
{
	DWORD r;
	co_rc_t rc = daemon->rc;

	*message_out = NULL;

	if (daemon->message) {
		*message_out = daemon->message;
		daemon->message = 0;
		return CO_RC(OK);
	}

	if (!CO_OK(rc))
		return rc;

	SetEvent(daemon->shifted);

	r = MsgWaitForMultipleObjects(1, &daemon->readable, FALSE, timeout,
				      QS_ALLEVENTS);
	if (r == WAIT_TIMEOUT || r == WAIT_OBJECT_0 + 1) {
		return CO_RC(TIMEOUT);
	}

	if (daemon->message) {
		*message_out = daemon->message;
		daemon->message = 0;
	}
	SetEvent(daemon->shifted);

	return CO_RC(OK);
}

co_rc_t
co_os_daemon_message_send(co_daemon_handle_t daemon, co_message_t * message)
{
	unsigned long bytes_written;
	unsigned long bytes_to_write = sizeof (*message) + message->size;
	BOOL ret;

	ret = WriteFile(daemon->handle, (char *) message,
			bytes_to_write, &bytes_written, NULL);

	if (ret != TRUE)
		return CO_RC(ERROR);

	if (bytes_written != bytes_to_write)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

co_rc_t
co_os_daemon_pipe_open(co_id_t linux_id, co_module_t module_id,
		       co_daemon_handle_t * handle_out)
{
	HANDLE handle = INVALID_HANDLE_VALUE;
	long written = 0;
	char pathname[0x100];
	co_daemon_handle_t daemon = 0;
	co_module_name_t module_name;

	snprintf(pathname, sizeof (pathname), "\\\\.\\pipe\\coLinux%d",
		 (int) linux_id);

	co_module_repr(module_id, &module_name);

	co_debug("connecting to instance %d as %s\n", linux_id, module_name);

	if (!WaitNamedPipe(pathname, NMPWAIT_USE_DEFAULT_WAIT)) {
		co_debug("connection timed out (%x)\n", GetLastError());
		goto co_os_open_daemon_pipe_error;
	}

	co_debug("connection established\n");

	handle = CreateFile(pathname,
			    GENERIC_READ | GENERIC_WRITE,
			    0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (handle == INVALID_HANDLE_VALUE)
		goto co_os_open_daemon_pipe_error;

	/* Identify ourselves to the daemon */
	if (!WriteFile(handle, &module_id, sizeof (module_id), &written, NULL)) {
		co_debug("attachment failed\n");
		goto co_os_open_daemon_pipe_error;
	}

	if (!(daemon = co_os_malloc(sizeof (*daemon))))
		goto co_os_open_daemon_pipe_error;

	daemon->handle = handle;
	daemon->loop = 1;
	daemon->rc = CO_RC(OK);
	daemon->message = 0;
	daemon->shifted = CreateEvent(0, FALSE, FALSE, 0);
	daemon->readable = CreateEvent(0, TRUE, TRUE, 0);
	if (!(daemon->shifted && daemon->readable))
		goto co_os_open_daemon_pipe_error;

	co_debug_lvl(pipe,10,"user daemon thread starting");
	daemon->thread =
	    CreateThread(0, 0, io_thread, daemon, 0, 0);

	if (daemon->thread) {
		*handle_out = daemon;
		return CO_RC(OK);
	}

	co_debug("co_os_daemon_thread() did not start\n");

	co_os_open_daemon_pipe_error:
	
	if (daemon) {
		if (daemon->readable)
			CloseHandle(daemon->readable);
		if (daemon->shifted)
			CloseHandle(daemon->shifted);
		if (daemon->thread)
			CloseHandle(daemon->thread);
		co_os_free(daemon);
	}

	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);

	return CO_RC(ERROR);
}

co_rc_t
co_os_daemon_pipe_close(co_daemon_handle_t daemon)
{
	daemon->loop = 0;
	SetEvent(daemon->shifted);
	WaitForSingleObject(daemon->thread, INFINITE);
	CloseHandle(daemon->readable);
	CloseHandle(daemon->shifted);
	CloseHandle(daemon->handle);
	CloseHandle(daemon->thread);
	HeapDestroy(daemon->heap);
	co_os_free(daemon);
	return CO_RC(OK);
}
