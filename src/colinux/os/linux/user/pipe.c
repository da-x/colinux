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
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/pipe.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "unix.h"
#include "frame.h"

struct co_os_pipe_connection {
	int sock;
	void *user_data;
	unsigned long index;
	co_os_frame_collector_t frame;
	bool_t deleted;
};

struct co_os_pipe_server {
	int sock;
	void *empty;
	char pathname[0x100];

	void *user_data;
	co_os_pipe_server_func_connected_t connected_func;
	co_os_pipe_server_func_packet_t packet_func;
	co_os_pipe_server_func_disconnected_t disconnected_func;

	unsigned long num_clients;
	co_os_pipe_connection_t *clients[CO_OS_PIPE_SERVER_MAX_CLIENTS];
	struct pollfd poll_array[CO_OS_PIPE_SERVER_MAX_CLIENTS + 1];
};

co_rc_t co_os_pipe_server_send(co_os_pipe_connection_t *conn, char *data, unsigned long size)
{
	return co_os_frame_send(conn->sock, data, size);
}

co_rc_t co_os_pipe_server_client_connected(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	return ps->connected_func(connection, ps->user_data, &connection->user_data);
}

co_rc_t co_os_pipe_server_client_disconnected(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection)
{
	ps->disconnected_func(connection, &connection->user_data);

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_client_create(co_os_pipe_server_t *ps, int sock)
{
	co_os_pipe_connection_t *connection;
	co_rc_t rc = CO_RC(OK);
	int index = 0;

	rc = co_os_set_blocking(sock, PFALSE);
	if (!CO_OK(rc))
		return rc;

	if (ps->num_clients == CO_OS_PIPE_SERVER_MAX_CLIENTS) {
		rc = CO_RC(ERROR);
		goto out;
	}

	connection = co_os_malloc(sizeof(*connection));
	if (connection == NULL) {
		rc = CO_RC(ERROR); 
		goto out;
	}
	
	memset(connection, 0, sizeof(*connection));

	connection->frame.max_buffer_size = CO_OS_PIPE_SERVER_BUFFER_SIZE;
	connection->sock = sock;

	rc = co_os_pipe_server_client_connected(ps, connection);
	if (!CO_OK(rc)) {
		goto out_free_connection;
	}

	index = ps->num_clients;
	ps->num_clients++;
	ps->clients[index] = connection;
	ps->poll_array[index+1].fd = sock;
	ps->poll_array[index+1].events = POLLIN | POLLHUP | POLLERR;
	ps->poll_array[index+1].revents = 0;

	return rc;
	/* error path */

out_free_connection:
	co_os_free(connection);

out:
	return rc;
}

void co_os_pipe_server_client_destroy(co_os_pipe_server_t *ps, co_os_pipe_connection_t *connection, int index)
{
	co_os_pipe_server_client_disconnected(ps, connection);
	close(connection->sock);
	co_os_free(connection);

	/* Move the last element to our place */
	ps->clients[index-1] = ps->clients[ps->num_clients-1];
	ps->clients[ps->num_clients-1] = NULL;
	ps->poll_array[index] = ps->poll_array[ps->num_clients];
	ps->num_clients--;
}

co_rc_t co_os_pipe_server_recv_client(co_os_pipe_server_t *ps, co_os_pipe_connection_t *conn)
{
	co_rc_t rc = CO_RC(OK);

	while (1) {
		char *data = NULL;
		unsigned long size = 0;

		rc = co_os_frame_recv(&conn->frame, conn->sock, &data, &size);
		if (!CO_OK(rc))
			return rc;

		if (data == NULL)
			break;

		rc = ps->packet_func(conn, &conn->user_data, data, size);
		co_os_free(data);

		if (!CO_OK(rc))
			break;
	}

	if (!CO_OK(rc))
		conn->deleted = PTRUE;

	return rc;
}

co_rc_t co_os_pipe_server_service(co_os_pipe_server_t *ps, bool_t infinite)
{
	int i, ret = 0;
	co_os_pipe_connection_t *conn;
	co_rc_t rc;
 
	for (i=1; i < ps->num_clients + 1; i++)
		ps->poll_array[i].revents = 0;
	

	ret = poll(ps->poll_array, ps->num_clients + 1, infinite ? 5 : 0);

	if (ps->poll_array[0].revents & POLLIN) {
		int sock;
		struct sockaddr_un saddr;
		size_t len = sizeof(saddr);

		sock = accept(ps->sock, (struct sockaddr *)&saddr, &len);
		if (sock != -1) {	
			co_rc_t rc;
			rc = co_os_pipe_server_client_create(ps, sock);
			if (!CO_OK(rc))
				close(sock);
		} else {
			return CO_RC(ERROR);
		}
	}

	for (i=1; i <= ps->num_clients; i++) {
		unsigned long revents = ps->poll_array[i].revents;

		conn = ps->clients[i-1];

		if (revents & POLLIN)
			rc = co_os_pipe_server_recv_client(ps, conn);

		if (revents & POLLHUP)
			conn->deleted = PTRUE;

		if (revents & POLLERR)
			conn->deleted = PTRUE;
	}

	i = 1;
	while (i <= ps->num_clients) {
		conn = ps->clients[i-1];
		if (conn->deleted) {
			co_os_pipe_server_client_destroy(ps, conn, i);
			continue;
		}
		i++;
	}

	return CO_RC(OK);
}

co_rc_t co_os_pipe_get_colinux_pipe_path(co_id_t instance, char *out_path, int size)
{
	char *home = getenv("HOME");

	if (home == NULL)
		home = "/tmp";

	snprintf(out_path, size, "%s/.colinux.%d.pipe", home, instance);

	return CO_RC(OK);
}

co_rc_t co_os_pipe_server_create(co_os_pipe_server_func_connected_t connected_func,
				 co_os_pipe_server_func_packet_t packet_func,
				 co_os_pipe_server_func_disconnected_t disconnected_func,
				 void *data,
				 co_os_pipe_server_t **ps_out,
				 co_id_t *id_out)
{
	co_os_pipe_server_t *ps;
	struct sockaddr_un saddr;
	struct stat st;
	int sock, ret;
	co_id_t id = 0;
	co_rc_t rc;

	saddr.sun_family = AF_UNIX;

	for (;;) {
		rc = co_os_pipe_get_colinux_pipe_path(id, saddr.sun_path, sizeof(saddr.sun_path));
		if (!CO_OK(rc))
			return rc;

		/*
		 * If the named pipe exists already, delete it if it cannot be opened
		 */
		if (stat(saddr.sun_path, &st) == 0) {
			sock = socket(AF_UNIX, SOCK_STREAM, 0);
			if (sock == -1)
				return CO_RC(ERROR);
			
			ret = connect(sock, (struct sockaddr *)&saddr, sizeof (saddr));
			if (ret < 0) {
				if (errno == ECONNREFUSED)
					unlink(saddr.sun_path);
			}
			else {
				close(ret);
				id++;
				continue;
			}
		}
		
		break;
	}

	*id_out = id;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		return CO_RC(ERROR);

	rc = co_os_set_blocking(sock, PFALSE);
	if (!CO_OK(rc)) {
		close(sock);
		return CO_RC(ERROR);
	}

	ret = bind(sock, (struct sockaddr *)&saddr, sizeof (saddr));
	if (ret == -1) {
		close(sock);
		return CO_RC(ERROR);
	}

	listen(sock, CO_OS_PIPE_SERVER_MAX_CLIENTS);

	ps = co_os_malloc(sizeof(*ps));
	if (!ps) {
		unlink(saddr.sun_path);
		return CO_RC(ERROR);
	}

	memset(ps, 0, sizeof(*ps));

	ps->sock = sock;
	snprintf(ps->pathname, sizeof(ps->pathname), "%s", saddr.sun_path);
	
	ps->user_data = data;
	ps->connected_func = connected_func;
	ps->packet_func = packet_func;
	ps->disconnected_func = disconnected_func;
	ps->poll_array[0].fd = sock;
	ps->poll_array[0].events = POLLIN;

	*ps_out = ps;

	return CO_RC(OK);
}

void co_os_pipe_server_destroy(co_os_pipe_server_t *ps)
{
	unlink(ps->pathname);
	co_os_free(ps);
}
