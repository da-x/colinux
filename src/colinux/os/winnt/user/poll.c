/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <windows.h>

#include <colinux/common/list.h>
#include <colinux/os/alloc.h>
#include <colinux/os/user/poll.h>
#include <colinux/user/monitor.h>

struct co_os_poll {
	HANDLE event;
	HANDLE finish;
	co_id_t id;
	co_monitor_ioctl_op_t poll_op;
	co_monitor_ioctl_op_t cancel_op;
	co_user_monitor_t *monitor;
	co_os_poll_callback_t callback;
	void *callback_data;
	co_list_t node;
	co_os_poll_chain_t chain;
};

struct co_os_poll_chain {
	int count;
	co_list_t list;
};


DWORD WINAPI co_os_poll_thread(LPVOID param)
{
	co_os_poll_t handle = (co_os_poll_t)param;
	co_user_monitor_t *monitor;
	co_rc_t rc;

	rc = co_user_monitor_open(handle->id, &monitor);
	if (!CO_OK(rc)) {
		co_debug("Error opening COLX driver\n");
		SetEvent(handle->finish);	
		return -1;
	}
	
	while (1) {
		rc = co_user_monitor_any(monitor, handle->poll_op);
		if (!CO_OK(rc))
			break;

		SetEvent(handle->event);
	}

	co_user_monitor_close(monitor);

	SetEvent(handle->finish);

	return 0;
}

extern co_rc_t co_os_poll_create(co_id_t id,
				 co_monitor_ioctl_op_t poll_op,
				 co_monitor_ioctl_op_t cancel_op,
				 co_os_poll_callback_t callback,
				 void *callback_data,
				 co_os_poll_chain_t chain,
				 co_os_poll_t *out_poll)
{
	co_os_poll_t handle;
	HANDLE thread_handle;
	DWORD thread_id;
	co_rc_t rc;

	handle = co_os_malloc(sizeof(*handle));
	if (!handle)
		return CO_RC(ERROR);

	rc = co_user_monitor_open(id, &handle->monitor);
	if (!CO_OK(rc)) {
		co_debug("Error opening COLX driver\n");
		goto error_out;
	}

	handle->event = CreateEvent(NULL, FALSE, FALSE, NULL);
	handle->finish = CreateEvent(NULL, FALSE, FALSE, NULL);
	handle->id = id;
	handle->poll_op = poll_op;
	handle->cancel_op = cancel_op;
	handle->callback = callback;
	handle->callback_data = callback_data;
	handle->chain = chain;

	co_list_add_tail(&handle->node, &chain->list);
	chain->count += 1;

	thread_handle = CreateThread(
		NULL, 0, &co_os_poll_thread,
		(LPVOID)handle, 0, &thread_id);

	*out_poll = handle;

	return CO_RC(OK);

error_out:
	co_os_free(handle);
	return rc;
}

void co_os_poll_destroy(co_os_poll_t handle)
{
	co_rc_t rc;

	rc = co_user_monitor_any(handle->monitor, handle->cancel_op);

	if (CO_OK(rc))
		WaitForSingleObject(handle->finish, INFINITE);

	SetEvent(handle->event);

	CloseHandle(handle->finish);
	CloseHandle(handle->event);

	co_user_monitor_close(handle->monitor);	

	handle->chain->count -= 1;
	co_list_del(&handle->node);
	
	co_os_free(handle);
}

co_rc_t co_os_poll_chain_create(co_os_poll_chain_t *chain_out)
{
	co_os_poll_chain_t handle;

	handle = co_os_malloc(sizeof(*handle));
	if (!handle)
		return CO_RC(ERROR);

	co_list_init(&handle->list);
	handle->count = 0;

	*chain_out = handle;

	return CO_RC(OK);
}

int co_os_poll_chain_wait(co_os_poll_chain_t chain)
{
	HANDLE wevents[chain->count];
	DWORD result; 
	co_os_poll_t poll;
	int i;
	
	i=0;
        co_list_each_entry(poll, &chain->list, node) {
		wevents[i] = poll->event;
		i++;
	}

	result = MsgWaitForMultipleObjects(chain->count, (void * const *)&wevents, 
					   FALSE, INFINITE, QS_ALLINPUT);

	if (result == WAIT_OBJECT_0 + chain->count)
		return 0;

	i=0;
        co_list_each_entry(poll, &chain->list, node) {
		if (i == result - WAIT_OBJECT_0)
			break;
		i++;
	}

	poll->callback(poll, poll->callback_data);

	return 1;
}

void co_os_poll_chain_destroy(co_os_poll_chain_t chain)
{
	co_os_poll_t poll;

	while (chain->count != 0) {
		poll = co_list_entry(chain->list.next, typeof(*poll), node);
		
		co_os_poll_destroy(poll);	
	}

	free(chain);
}

#if (0)
typedef struct {
	HANDLE pipe;
	OVERLAPPED overlap;
} *co_os_pipe_server_t;

co_rc_t co_os_create_server_pipe(int id, co_pipe_server_t *server)
{
	LPTSTR pipename = "\\\\.\\pipe\\coLinux0"; 
	BOOL ret;

	server->overlap.hEvent = CreateEvent(NULL,  TRUE, FALSE, NULL);

	server->pipe = CreateNamedPipe(
		pipename,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0x100000,
		0x100000,
		1000,
		NULL);

	ret = ConnectNamedPipe(server->pipe, &server->overlap);

	return 0;
}
#endif
