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
 * Maximum/Minimum memory size of packet allocation heap.
 */

#define PACKET_HEAP_SIZE_MIN 0x10000
#define PACKET_HEAP_SIZE_MAX 0x100000

/*
 * Queue iteration threshold for packet retention and deallocation
 */
 
#define PACKET_RETENTION 3

/*
 * Intraprocess synchronization
 */
 
static CRITICAL_SECTION io_sync;
 
/*
 * Data packet structure with single-linked list
 */
 
typedef struct _packet {
	struct _packet *next;
	unsigned size;
	unsigned free;			// ==0 allocated, !=0 iteration count
	co_message_t message[0];
} packet_t;

/*
 * Finite state enumeration of I/O operations in io_thread()
 */

typedef enum {
	io_read = 1,
	io_read_submit,
	io_wait,
	io_wait_resources,
	io_read_more,
	io_read_burst,
	io_read_packet,
	io_queue,
	io_queue_burst,
	io_queue_burst_loop,
	io_error
} io_state;

/*
 * Data passed between asynchronous procedure calls and io_thread()
 */

typedef struct {
	co_daemon_handle_t daemon;
	co_rc_t rc;
	io_state iostate;
	io_state return_state;
	unsigned transferred;
	unsigned transfer;
} io_data;

static packet_t *
packet_allocate(co_daemon_handle_t daemon, size_t size)
{
	packet_t *packet = daemon->packets;
	packet_t **shadow = (packet_t**)&daemon->packets;

	EnterCriticalSection(&io_sync);
	while(packet) {
		if(packet->free) {
			if(packet->size >= size && size >= (packet->size >> 1)) {
				packet->free = 0;
				*shadow = packet->next;
				packet->next = 0;
				LeaveCriticalSection(&io_sync);
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
	LeaveCriticalSection(&io_sync);

	if(!(packet = HeapAlloc(daemon->heap, HEAP_NO_SERIALIZE, sizeof(packet_t)+size)))
		return NULL;
	packet->next = 0;
	packet->free = 0;
	packet->size = size;
	return packet;
}

static bool_t
packet_reallocate(co_daemon_handle_t daemon, packet_t **packet, size_t size)
{
	// UPDATE: reallocate within range and 
	if((*packet)->size >= size)
		return PTRUE;
	if(HeapReAlloc(daemon->heap, HEAP_REALLOC_IN_PLACE_ONLY, *packet, sizeof(packet_t)+size)) {
		(*packet)->size = size;
		return PTRUE;
	}
	packet_t *newpacket;
	if(!(newpacket = packet_allocate(daemon, size)))
		return PFALSE;
	memcpy(newpacket->message, (*packet)->message, (*packet)->size);
	EnterCriticalSection(&io_sync);
	(*packet)->free = 1;
	(*packet)->next = daemon->packets;
	daemon->packets = *packet;
	LeaveCriticalSection(&io_sync);
	*packet = newpacket;
	return PTRUE;
}

static void CALLBACK io_readCompletion(DWORD err, DWORD transferred, LPOVERLAPPED overlapped)
{
	io_data *iodata = (io_data*)overlapped->hEvent;
	if (!transferred) {
		if (err == ERROR_BROKEN_PIPE) {
			iodata->iostate = io_error;
			iodata->rc = CO_RC(BROKEN_PIPE);
			return;
		}
		if (err != ERROR_SUCCESS) {
			co_debug_lvl(pipe,10,"io error %x",err);
			iodata->iostate = io_error;
			iodata->rc = CO_RC(ERROR);
			return;
		}
	}
	if ((iodata->transferred += transferred) > iodata->transfer) {
		co_debug_lvl(pipe,10,"io transfer larger than requested itd=%d td=%d t=%d", iodata->transferred, transferred, iodata->transfer);
		iodata->iostate = io_error;
		iodata->rc = CO_RC(ERROR);
		return;
	}
	iodata->iostate = iodata->return_state;
	iodata->rc = CO_RC(OK);
}


static DWORD WINAPI io_thread(LPVOID param)
{
	co_daemon_handle_t daemon = param;
	HANDLE pipe = daemon->handle;
	OVERLAPPED overlapped;
	bool_t async = PFALSE;
	packet_t *packet = 0;
	packet_t *p = 0;
	co_message_t *message = 0;
	io_state iostate;
	io_state return_state = 0;
	io_data iodata;
	co_rc_t rc;
	DWORD size;
	DWORD code;
	unsigned burst_size = 0;

	co_debug_lvl(pipe,10,"io thread started");
	daemon->heap = HeapCreate(HEAP_NO_SERIALIZE, PACKET_HEAP_SIZE_MIN, PACKET_HEAP_SIZE_MAX);
//	HeapSetInformation(daemon->heap, HeapCompatibilityInformation, &(unsigned long)2, sizeof(unsigned long);
	memset(&overlapped, 0, sizeof (overlapped));
	overlapped.hEvent = &iodata;
	iodata.iostate = io_wait;
	iodata.daemon = daemon;
	rc = CO_RC(OK);

	iostate = io_wait;
	while (daemon->loop) {
		switch(iostate) {
			case io_read: {
				if (!(packet = packet_allocate(daemon,sizeof(co_message_t)))) {
					return_state = io_read;
					iostate = io_wait_resources;
					continue;
				}
				iodata.transferred = 0;
				iodata.transfer = sizeof(co_message_t);
				iodata.return_state = io_read_more;
			}
			case io_read_submit: {
				if (!ReadFileEx(pipe, (void*)packet->message + iodata.transferred, iodata.transfer - iodata.transferred, &overlapped, io_readCompletion)) {
					co_debug_lvl(pipe,10,"io read error %d", GetLastError());
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				async = PTRUE;
				iostate = io_wait;
			}
			case io_wait: {
				code = WaitForSingleObjectEx(daemon->shifted, INFINITE, TRUE);
				if(code == WAIT_FAILED) {
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				if(code == WAIT_IO_COMPLETION && iodata.iostate) {
					async = PFALSE;
					if ((iostate = iodata.iostate) == io_error) {
						rc = iodata.rc;
						break;
					}
					iodata.iostate = 0;
					continue;
				}
				if(!async)
					iostate = io_read;
				continue;
			}
			case io_wait_resources: {
				HeapCompact(daemon->heap, HEAP_NO_SERIALIZE);
				code = WaitForSingleObject(daemon->shifted, 10);  // UPDATE: avoid busy wait: timeout
				if(code == WAIT_FAILED) {
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				iostate = return_state;
				continue;
			}
			case io_read_more: {
				if (!PeekNamedPipe(pipe, 0, 0, 0, &size, 0)) {
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				if ((iodata.transfer += size) <= packet->message->size) {
					iostate = io_read_packet;
					continue;
				}
				burst_size = PACKET_HEAP_SIZE_MAX >> 1;
				if (iodata.transfer > burst_size)
					iodata.transfer = burst_size;
			}
			case io_read_burst: {
				if (!packet_reallocate(daemon, &packet, iodata.transfer)) {
					if ((burst_size >>= 1) < PACKET_HEAP_SIZE_MIN) {
						iodata.transfer = sizeof(co_message_t) + packet->message->size;
						return_state = io_read_packet;
						iostate = io_wait_resources;
						continue;
					}
					if (iodata.transfer > burst_size)
						iodata.transfer = burst_size;
					return_state = io_read_burst;
					iostate = io_wait_resources;
					continue;
				}
				iodata.return_state = io_queue_burst;
				iostate = io_read_submit;
				continue;
			}
			case io_read_packet: {
				if (!packet_reallocate(daemon, &packet, iodata.transfer)) {
					return_state = io_read_packet;
					iostate = io_wait_resources;
					continue;
				}
				iodata.return_state = io_queue;
				iostate = io_read_submit;
				continue;
			}
			case io_queue: {
				EnterCriticalSection(&io_sync);
				if(!daemon->qOut)
					daemon->qOut = daemon->qIn = packet;
				else {
					((packet_t*)daemon->qIn)->next = packet;
					daemon->qIn = packet;
				}
				if (!SetEvent(daemon->readable)) {
					LeaveCriticalSection(&io_sync);
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				LeaveCriticalSection(&io_sync);
				iostate = io_read;
				continue;
			}
			case io_queue_burst: {
				size = 0;
				message = packet->message;
				iostate = io_queue_burst_loop;
			}
			case io_queue_burst_loop: {
				if (!(p = packet_allocate(daemon,sizeof(co_message_t)+message->size))) {
					return_state = io_queue_burst_loop;
					iostate = io_wait_resources;
					continue;
				}
				memcpy(p->message, message, sizeof(co_message_t)+message->size);
				EnterCriticalSection(&io_sync);
				if (!daemon->qOut)
					daemon->qOut = daemon->qIn = p;
				else {
					((packet_t*)daemon->qIn)->next = p;
					daemon->qIn = p;
				}
				if (!SetEvent(daemon->readable)) {
					LeaveCriticalSection(&io_sync);
					iostate = io_error;
					rc = CO_RC(ERROR);
					break;
				}
				if ((size += message->size + sizeof(co_message_t)) == iodata.transferred) {
					packet->free = 1;
					packet->next = daemon->packets;
					daemon->packets = packet;
					LeaveCriticalSection(&io_sync);
					// packet = 0;
					iostate = io_read;
					continue;
				}
				LeaveCriticalSection(&io_sync);
				message = (void*)packet->message + size;
				if (size + sizeof(co_message_t) <= iodata.transferred)
					if (size + sizeof(co_message_t) + message->size <= iodata.transferred) {
						iostate = io_queue_burst_loop;
						continue;
					}
				iodata.transferred -= size;
				memcpy(packet->message, message, iodata.transferred);
				if (iodata.transferred >= sizeof(co_message_t)) {
					iodata.transfer = iodata.transferred;
					iostate = io_read_more;
					continue;
				}
				iodata.transfer = sizeof(co_message_t);
				iodata.return_state = io_read_more;
				iostate = io_read_submit;
				continue;
			}
			case io_error: break;
		}
		if(iostate == io_error)
			break;
	}

	if (async) {
		CancelIo(pipe);
		async = 0;
	}
	daemon->rc = rc;
	if (daemon->loop)
		if ((!SetEvent(daemon->readable)) && CO_OK(rc))
			co_debug_lvl(pipe,1,"error");
	return 0;
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
	co_rc_t rc = daemon->rc;
	packet_t *packet;
	DWORD code;

	*message_out = NULL;

	if((!daemon->qOut) && CO_OK(rc)) {
		code = MsgWaitForMultipleObjects(1, &daemon->readable, FALSE, timeout, QS_ALLEVENTS);
		if (code == WAIT_OBJECT_0 + 1)
			return CO_RC(TIMEOUT);
		if (code == WAIT_TIMEOUT) {
			SetEvent(daemon->shifted);
			return CO_RC(TIMEOUT);
		}
	}
	if(daemon->qOut) {
		EnterCriticalSection(&io_sync);
		packet = daemon->qOut;
		daemon->qOut = packet->next;
		packet->next = daemon->packets;
		daemon->packets = packet;
		*message_out = packet->message;
		if(!daemon->qOut) {
			if (!ResetEvent(daemon->readable)) {
				LeaveCriticalSection(&io_sync);
				return CO_RC(ERROR);
			}
//			SetEvent(daemon->shifted);
		}
		LeaveCriticalSection(&io_sync);
		return CO_RC(OK);
	}
	SetEvent(daemon->shifted);
	return rc;
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
        long lpMode = PIPE_READMODE_MESSAGE;

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

	InitializeCriticalSection(&io_sync);

	if (!(daemon = co_os_malloc(sizeof (*daemon))))
		goto co_os_open_daemon_pipe_error;

	daemon->handle = handle;
	daemon->loop = 1;
	daemon->rc = CO_RC(OK);
	daemon->shifted = CreateEvent(0, FALSE, FALSE, 0);
	daemon->readable = CreateEvent(0, TRUE, TRUE, 0);
	daemon->qIn = daemon->qOut = daemon->packets = 0;
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
	DeleteCriticalSection(&io_sync);
	return CO_RC(OK);
}
