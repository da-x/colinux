/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_OS_USER_PIPE_H__
#define __COLINUX_OS_USER_PIPE_H__

#include <colinux/common/common.h>

#define CO_OS_PIPE_SERVER_MAX_CLIENTS 100
#define CO_OS_PIPE_SERVER_BUFFER_SIZE 0x10000

struct co_os_pipe_connection;
typedef struct co_os_pipe_connection co_os_pipe_connection_t;

struct co_os_pipe_server;
typedef struct co_os_pipe_server co_os_pipe_server_t;

typedef co_rc_t (*co_os_pipe_server_func_connected_t)(co_os_pipe_connection_t *conn,
						   void *data, 
						   void **data_client);


typedef co_rc_t (*co_os_pipe_server_func_packet_t)(co_os_pipe_connection_t *conn, 
						   void **data,
						   char *packet_data,
						   unsigned long size);

typedef void (*co_os_pipe_server_func_disconnected_t)(co_os_pipe_connection_t *conn,
						      void **data);


extern co_rc_t co_os_pipe_server_create(co_os_pipe_server_func_connected_t connected_func,
					co_os_pipe_server_func_packet_t packet_func,
					co_os_pipe_server_func_disconnected_t disconnected_func,
					void *data,
					co_os_pipe_server_t **ps_out);

extern void co_os_pipe_server_destroy(co_os_pipe_server_t *ps);

extern co_rc_t co_os_pipe_server_send(co_os_pipe_connection_t *conn, char *data, unsigned long size);

extern co_rc_t co_os_pipe_server_service(co_os_pipe_server_t *ps, 
					 bool_t infinite);

#endif
