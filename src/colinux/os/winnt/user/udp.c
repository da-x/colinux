/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Service support by Jaroslaw Kowalski <jaak@zd.com.pl>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <winsock2.h>

#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/misc.h>

int co_udp_socket_connect(const char *addr, unsigned short int port)
{
	struct sockaddr_in server;
	int ret, sock;
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2,2), &wsaData);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		printf("unable to create socket %x\n", sock);
		return -1;
	}

	co_bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(addr);
	server.sin_port = htons(port);

	ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
	if (ret)
		return -1;

	return sock;
}

int co_udp_socket_send(int sock, const char *buffer, unsigned long size)
{
	return send(sock, buffer, size, 0);
}

void co_udp_socket_close(int sock)
{
	closesocket(sock);
}

