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
#include <stdarg.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <colinux/common/libc.h>
#include <colinux/user/debug.h>

void co_daemon_trace_point(co_trace_point_info_t *info)
{
#ifdef COLINUX_DEBUG
	/*
	 * This code sends UDP packet per debug line.
	 *
	 * Proves useful for investigating hard crashes.
	 *
	 * Make sure you have a fast Ethernet.
	 */

	static int sock = -1;    /* We only need one global socket */
	if (sock == -1) {
		struct sockaddr_in server;
		int ret;
		sock = socket(AF_INET, SOCK_DGRAM, 0);

		co_bzero((char *) &server, sizeof(server));
		server.sin_family = AF_INET;

		/*
		 * Hardcoded for the meanwhile.
		 *
		 * If someone actually uses this, please send a patch
		 * to make this more configurable.
		 */
		server.sin_addr.s_addr = inet_addr("192.168.1.1");
		server.sin_port = htons(5555);

		ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
	}

	send(sock, info, sizeof(*info), 0);
#endif
}
