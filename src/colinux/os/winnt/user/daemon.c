/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/*
 * Ballard, Jonathan H.  <jhballard@hotmail.com>
 *  20040222 : Designed and implemented co_os_daemon_thread()
 *           : with message queue, wait state, and error
 *           : recovery.
 *  20050117 : Updated message queue to use co_message_packet_t,
 *           : allocate_message() and co_os_daemon_deallocate_message()
 */

#include <windows.h>
#include <stdio.h>

#include "daemon.h"

#include <colinux/os/alloc.h>
#include <colinux/common/messages.h>

static DWORD WINAPI co_os_daemon_thread(LPVOID data);

typedef struct co_message_packet {
	struct co_message_packet *next;
	unsigned size;
	unsigned free;
	co_message_t data[0];
} co_message_packet_t;

co_rc_t
co_os_open_daemon_pipe(co_id_t linux_id, co_module_t module_id,
		       co_daemon_handle_t * handle_out)
{
	HANDLE handle = INVALID_HANDLE_VALUE;
	long written = 0;
	char pathname[0x100];
	co_daemon_handle_t daemon_handle = 0;
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

	if (!(daemon_handle = co_os_malloc(sizeof (*daemon_handle))))
		goto co_os_open_daemon_pipe_error;

	daemon_handle->handle = handle;
	daemon_handle->loop = 1;
	daemon_handle->rc = CO_RC(OK);
	daemon_handle->message = 0;
	daemon_handle->shifted = CreateEvent(0, FALSE, FALSE, 0);
	daemon_handle->readable = CreateEvent(0, TRUE, TRUE, 0);
	if (!(daemon_handle->shifted && daemon_handle->readable))
		goto co_os_open_daemon_pipe_error;

	co_debug_lvl(pipe,10,"user daemon thread starting");
	daemon_handle->thread =
	    CreateThread(0, 0, co_os_daemon_thread, daemon_handle, 0, 0);

	if (daemon_handle->thread) {
		*handle_out = daemon_handle;
		return CO_RC(OK);
	}

	co_debug("co_os_daemon_thread() did not start\n");

	co_os_open_daemon_pipe_error:
	
	if (daemon_handle) {
		if (daemon_handle->readable)
			CloseHandle(daemon_handle->readable);
		if (daemon_handle->shifted)
			CloseHandle(daemon_handle->shifted);
		if (daemon_handle->thread)
			CloseHandle(daemon_handle->thread);
		co_os_free(daemon_handle);
	}

	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);

	return CO_RC(ERROR);
}

static co_message_packet_t *
allocate_message(co_daemon_handle_t D, size_t size)
{
	co_message_packet_t *m = D->packets;
	co_message_packet_t **p = (co_message_packet_t**)&D->packets;

	while(m) {
		if(m->free) {
			if(m->size >= size && size >= (m->size >> 1)) {
				m->free = 0;
				*p = m->next;
				m->next = 0;
				return m;
			}
			if(++m->free <= 3) {  // retention threshold
				p = &m->next;
				m = m->next;
				continue;
			}
			*p = m->next;
			HeapFree(D->heap, HEAP_NO_SERIALIZE, m);
//			co_os_free(m);
			m = *p;
			continue;
		}
		p = &m->next;
		m = m->next;
	}

	m = HeapAlloc(D->heap, HEAP_NO_SERIALIZE, sizeof(struct co_message_packet)+size);
//	m = co_os_malloc(sizeof(struct co_message_packet)+size);
	if(!m)
		return NULL;
	m->next = 0;
	m->free = 0;
	m->size = size;
	return m;
}

void
co_os_daemon_deallocate_message(co_message_t *m)
{
	co_message_packet_t *p;
	
	p = (co_message_packet_t*)((unsigned)m - sizeof(struct co_message_packet));
	p->free = 1;
}

co_rc_t
co_os_daemon_get_message(co_daemon_handle_t handle,
			 co_message_t ** message_out, unsigned long timeout)
{
	DWORD r;
	co_rc_t rc = handle->rc;

	*message_out = NULL;

	if (handle->message) {
		*message_out = handle->message;
		handle->message = 0;
		return CO_RC(OK);
	}

	if (!CO_OK(rc))
		return rc;

	SetEvent(handle->shifted);

	r = MsgWaitForMultipleObjects(1, &handle->readable, FALSE, timeout,
				      QS_ALLEVENTS);
	if (r == WAIT_TIMEOUT || r == WAIT_OBJECT_0 + 1) {
		return CO_RC(TIMEOUT);
	}

	if (handle->message) {
		*message_out = handle->message;
		handle->message = 0;
	}
	SetEvent(handle->shifted);

	return CO_RC(OK);
}

co_rc_t
co_os_daemon_send_message(co_daemon_handle_t handle, co_message_t * message)
{
	unsigned long bytes_written;
	unsigned long bytes_to_write = sizeof (*message) + message->size;
	BOOL ret;

	ret = WriteFile(handle->handle, (char *) message,
			bytes_to_write, &bytes_written, NULL);

	if (ret != TRUE)
		return CO_RC(ERROR);

	if (bytes_written != bytes_to_write)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

static DWORD WINAPI
co_os_daemon_thread(LPVOID D)
{
	co_daemon_handle_t d = D;
	HANDLE h = d->handle;
	HANDLE w[2];
	OVERLAPPED overlapped;
	DWORD r;
	co_message_packet_t *data = 0;
	co_message_packet_t *p = 0;
	bool_t async = PFALSE;
	co_rc_t rc;

#	define NT_ERROR(_1) { \
		co_debug_lvl(pipe,1,"co_os_daemon_thread() error [ %x , %s ]\n", \
			GetLastError(), \
			_1); \
		goto co_os_daemon_thread_error; \
	}
	
	d->qIn = d->qOut = d->packets = 0;
	co_debug_lvl(pipe,10,"user daemon thread started");
	d->heap = HeapCreate(HEAP_NO_SERIALIZE, 0x10000, 0x100000);

	memset(&overlapped, 0, sizeof (overlapped));
	overlapped.hEvent = CreateEvent(0, TRUE, FALSE, 0);

	if (!overlapped.hEvent)
		NT_ERROR("CE001");

	w[0] = overlapped.hEvent;
	w[1] = d->shifted;

      co_os_daemon_thread_loop:
	if (WaitForSingleObject(w[1], INFINITE) == WAIT_FAILED)
		NT_ERROR("W0001");

	while (d->loop) {
		if ((!d->message) && d->qOut) {
			p = d->qOut;
			if(! (d->qOut = p->next) )
				d->qIn = 0;
			p->next = d->packets;
			d->packets = p;
			d->message = p->data;
			if (!SetEvent(d->readable))
				NT_ERROR("SE0001");
		}
		if (async) {
			r = WaitForMultipleObjects(2, w, FALSE, INFINITE);
			if(r == WAIT_FAILED)
				NT_ERROR("W0002");
			if (!d->message)
				if (!ResetEvent(d->readable))
					NT_ERROR("RE0001");
			if (r != WAIT_OBJECT_0)
				continue;
			if (!GetOverlappedResult(h, &overlapped, &r, FALSE)) {
				r = GetLastError();
				if (r == ERROR_IO_INCOMPLETE)
					continue;
				if (r == ERROR_IO_PENDING)
					continue;
				if (r == ERROR_BROKEN_PIPE)
					goto co_os_daemon_thread_broken_pipe;
				NT_ERROR("GOR01");
			}
			if(!r)	{
				data->free = 1;
				data->next = d->packets;
				d->packets = data;
				}
			else	{
				if(r < sizeof(co_message_t))
					NT_ERROR("DS0001");
				if(r != data->data->size + sizeof(co_message_t))
					NT_ERROR("DS0002");
				if(!d->qOut) {
					d->qOut = d->qIn = data;
				} else {
					((co_message_packet_t*)d->qIn)->next = data;
					d->qIn = data;
				}
			}
			data = 0;
			async = PFALSE;
		}
		if (!async) {
			if (!PeekNamedPipe(h, 0, 0, 0, 0, &r))
				NT_ERROR("PNP01");
			if (!(data = allocate_message(d,r)))
				goto co_os_daemon_thread_loop;
			async = PTRUE;
			if (!ReadFile(h, data->data, r, 0, &overlapped)) {
				r = GetLastError();
				if (r == ERROR_IO_INCOMPLETE)
					continue;
				if (r == ERROR_IO_PENDING)
					continue;
				if (r == ERROR_BROKEN_PIPE)
					goto co_os_daemon_thread_broken_pipe;
				NT_ERROR("RF001");
			}
		}
	}

	rc = CO_RC(OK);

	co_os_daemon_thread_return:
	if (async) {
		CancelIo(h);
		HeapFree(d->heap, HEAP_NO_SERIALIZE, data);
//		co_os_free(data);
		async = 0;
	}
	while (d->loop && d->qOut) {
		if (!d->message) {
			p = d->qOut;
			if(! (d->qOut = p->next) )
				d->qIn = 0;
			p->next = d->packets;
			d->packets = p;
			d->message = p->data;
			if (!SetEvent(d->readable)) {
				if (!CO_OK(rc))
					break;
				NT_ERROR("SE0002");
			}
		}
		if (WaitForSingleObject(w[1], INFINITE) == WAIT_FAILED) {
			if (!CO_OK(rc))
				break;
			NT_ERROR("W0003");
		}
		if (!d->message)
			if (!ResetEvent(d->readable)) {
				if (!CO_OK(rc))
					break;
				NT_ERROR("RE0002");
			}
	}
	if(d->packets) {
		data = d->packets;
		do {
			p = data->next;
			HeapFree(d->heap, HEAP_NO_SERIALIZE, data);
//			co_os_free(data);
		} while( (data = p) );
	}
	if (overlapped.hEvent)
		if ((!CloseHandle(overlapped.hEvent)) && CO_OK(rc))
			co_debug_lvl(pipe,1,"co_os_daemon_thread() error"
				 " [ %x , CH0001 ]\n", GetLastError());
	d->rc = rc;
	if (d->loop)
		if ((!SetEvent(d->readable)) && CO_OK(rc))
			co_debug_lvl(pipe,1,"co_os_daemon_thread() error"
				 " [ %x , SE0003 ]\n", GetLastError());

	HeapDestroy(d->heap);
	return 0;

      co_os_daemon_thread_error:
	rc = CO_RC(ERROR);
	goto co_os_daemon_thread_return;

      co_os_daemon_thread_broken_pipe:
	rc = CO_RC(BROKEN_PIPE);
	goto co_os_daemon_thread_return;

#	undef Q_REALLOC_1
#	undef Q_REALLOC_2
#	undef Q_COPY_2
#	undef NT_ERROR
}

void
co_os_daemon_close(co_daemon_handle_t D)
{
	D->loop = 0;
	SetEvent(D->shifted);
	WaitForSingleObject(D->thread, INFINITE);
	CloseHandle(D->readable);
	CloseHandle(D->shifted);
	CloseHandle(D->handle);
	CloseHandle(D->thread);
	co_os_free(D);
}
