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

#include <colinux/os/alloc.h>
#include <colinux/os/user/pipe.h>

typedef enum {
	CO_OS_PIPE_SERVER_STATE_NOT_CONNECTED,
	CO_OS_PIPE_SERVER_STATE_CONNECTED,
} co_os_pipe_server_state_t;

struct co_os_pipe_connection {
	OVERLAPPED overlap;
	HANDLE pipe;
	co_os_pipe_server_state_t state;
	char *in_buffer;
	unsigned long buffer_size;
	void *user_data;
	unsigned long index;
};

struct co_os_pipe_server {
	void *user_data;
	co_os_pipe_server_func_connected_t connected_func;
	co_os_pipe_server_func_packet_t packet_func;
	co_os_pipe_server_func_disconnected_t disconnected_func;
	unsigned long num_clients;

	co_os_pipe_connection_t *clients[CO_OS_PIPE_SERVER_MAX_CLIENTS];
	HANDLE events[CO_OS_PIPE_SERVER_MAX_CLIENTS];
};

co_rc_t co_os_pipe_server_create_client(co_os_pipe_server_t *ps);
co_rc_t co_os_pipe_server_destroy_client(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection);

co_rc_t co_os_pipe_server_handle_packet(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection, 
					unsigned long size)
{
	return ps->packet_func(connection, &connection->user_data, connection->in_buffer, size);
}

co_rc_t co_os_pipe_server_send(co_os_pipe_connection_t *conn, char *data, unsigned long size)
{
	unsigned long written = 0;
	BOOL ret;

	/* Current we don't queue packets here. Let WriteFile block... */
	ret = WriteFile(conn->pipe, data, size, &written, NULL); 
	if (!ret)
		return CO_RC(ERROR);
	
	if (written != size)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_client_connected(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	connection->state = CO_OS_PIPE_SERVER_STATE_CONNECTED;

	co_os_pipe_server_create_client(ps);

	return ps->connected_func(connection, ps->user_data, &connection->user_data);
}

co_rc_t co_os_pipe_server_client_disconnected(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	ps->disconnected_func(connection, &connection->user_data);

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_read_sync(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	unsigned long read_size;
	BOOL result;

	while (1) {
		result = ReadFile(connection->pipe,
				  connection->in_buffer,
				  connection->buffer_size,
				  &read_size,
				  &connection->overlap);

		if (!result) { 
			DWORD error = GetLastError();
		
			switch (error)
			{ 
			case ERROR_IO_PENDING: 
				return CO_RC(OK);
			case ERROR_BROKEN_PIPE:
				return co_os_pipe_server_destroy_client(ps, connection);
			default:
				return CO_RC(ERROR);
			} 
		}

		co_os_pipe_server_handle_packet(ps, connection, read_size);
	}

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_read_async(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	BOOL result;
	unsigned long read_size;

	if (connection->state == CO_OS_PIPE_SERVER_STATE_NOT_CONNECTED) {
		co_os_pipe_server_client_connected(ps, connection);
		return co_os_pipe_server_read_sync(ps, connection);
	}

	result = GetOverlappedResult(
		connection->pipe,
		&connection->overlap,
		&read_size,
		FALSE);
	
	if (result) {
		co_os_pipe_server_handle_packet(ps, connection, read_size);
		co_os_pipe_server_read_sync(ps, connection);
	} else {
		switch (GetLastError()) {
			case ERROR_BROKEN_PIPE:
				return co_os_pipe_server_destroy_client(ps, connection);
		}
	}

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_destroy_client(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	unsigned long index;

	if (connection->state == CO_OS_PIPE_SERVER_STATE_CONNECTED) {
		co_os_pipe_server_client_disconnected(ps, connection);
	}

	index = connection->index;

	CloseHandle(connection->pipe);
	CloseHandle(ps->events[index]);
	co_os_free(connection->in_buffer);
	co_os_free(connection);

	/* 
	 * If we are not deleting the last used entry, put the last 
	 * used entry in our place, so the list has no holes.
	 */

	if (index < ps->num_clients - 1) {
		ps->events[index] = ps->events[ps->num_clients - 1];
		ps->clients[index] = ps->clients[ps->num_clients - 1];
		ps->clients[index]->index = index;
	}

	ps->num_clients--;

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_create_client(co_os_pipe_server_t *ps)
{
	LPTSTR pipename = "\\\\.\\pipe\\coLinux0"; 
	co_os_pipe_connection_t *connection;
	HANDLE event;
	int index;
	co_rc_t rc = CO_RC(OK);

	if (CO_OS_PIPE_SERVER_MAX_CLIENTS == ps->num_clients) {
		rc = CO_RC(ERROR); 
		goto out;
	}

	connection = co_os_malloc(sizeof(*connection));
	if (connection == NULL) {
		rc = CO_RC(ERROR); 
		goto out;
	}

	connection->in_buffer = (char *)co_os_malloc(CO_OS_PIPE_SERVER_BUFFER_SIZE);
	if (connection->in_buffer == NULL) {
		rc = CO_RC(ERROR);
		goto out_free_connection;
	}

	connection->buffer_size = CO_OS_PIPE_SERVER_BUFFER_SIZE;

	event = CreateEvent(NULL,  TRUE, FALSE, NULL);
	if (event == INVALID_HANDLE_VALUE) {
		rc = CO_RC(ERROR);
		goto out_free_buffer;
	}
	ResetEvent(event);

	connection->pipe = CreateNamedPipe(
		pipename,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		0x100000,
		0x100000,
		1000,
		NULL);

	if (connection->pipe == INVALID_HANDLE_VALUE) {
		rc = CO_RC(ERROR);
		goto out_close_event;
	}

	index = ps->num_clients;	
	connection->overlap.hEvent = event;
	connection->index = index;
	ps->num_clients++;
	ps->clients[index] = connection;
	ps->events[index] = event;

	if (ConnectNamedPipe(connection->pipe, &connection->overlap) != 0) {
		co_os_pipe_server_client_connected(ps, connection);
		rc = co_os_pipe_server_read_sync(ps, connection);
	} else {
		connection->state = CO_OS_PIPE_SERVER_STATE_NOT_CONNECTED;
	}

	return rc;
	
	/* error path */
out_close_event:
	CloseHandle(event);

out_free_buffer:
	co_os_free(connection->in_buffer);

out_free_connection:
	co_os_free(connection);

out:
	return rc;
}

co_rc_t co_os_pipe_server_service(co_os_pipe_server_t *ps, bool_t infinite)
{
	DWORD result;
	co_rc_t rc;

	if (ps->num_clients == 0) {
		rc = co_os_pipe_server_create_client(ps);
		if (!CO_OK(rc))
			return rc;
	}

	result = MsgWaitForMultipleObjects(ps->num_clients,
					   ps->events,
					   FALSE,
					   infinite ? 10 : 0,
					   QS_ALLEVENTS);

	if ((result >= WAIT_OBJECT_0)  && 
	    (result < WAIT_OBJECT_0 + ps->num_clients)) 
	{
		long index = result - WAIT_OBJECT_0;

		ResetEvent(ps->events[index]);
			
		co_os_pipe_server_read_async(ps, ps->clients[index]);

	} else if (result == WAIT_OBJECT_0 + ps->num_clients) {
		MSG msg; 

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg); 
		}
	}

	return CO_RC(OK);
}


co_rc_t co_os_pipe_server_create(co_os_pipe_server_func_connected_t connected_func,
				 co_os_pipe_server_func_packet_t packet_func,
				 co_os_pipe_server_func_disconnected_t disconnected_func,
				 void *data,
				 co_os_pipe_server_t **ps_out)
{
	co_os_pipe_server_t *ps;

	ps = co_os_malloc(sizeof(*ps));
	if (!ps)
		return CO_RC(ERROR);

	memset(ps, 0, sizeof(*ps));

	*ps_out = ps;

	ps->packet_func = packet_func;
	ps->connected_func = connected_func;
	ps->disconnected_func = disconnected_func;
	ps->user_data = data;

	return CO_RC(OK);
}

void co_os_pipe_server_destroy(co_os_pipe_server_t *ps)
{
	/* While there are clients, delete the first one */

	while (ps->num_clients) {
		co_os_pipe_server_destroy_client(ps, ps->clients[0]);
	}

	co_os_free(ps);
}
