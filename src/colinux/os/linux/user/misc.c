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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>
#include <colinux/os/user/misc.h>

void co_terminal_print(const char *format, ...)
{
	char buf[0x100];
	va_list ap;
	int len;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	printf("%s: %s", _colinux_module, buf);

	len = strlen(buf);
	while (len > 0  &&  buf[len-1] == '\n')
		buf[len - 1] = '\0';
		
	co_debug_lvl(prints, 11, "prints \"%s\"\n", buf);
}

double co_os_timer_highres()
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return ((double)tv.tv_sec) + ((double)tv.tv_usec)/1000000;
}

void co_os_get_timestamp(co_timestamp_t *dts)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	dts->high = tv.tv_sec;
        dts->low = tv.tv_usec;
}

int co_udp_socket_connect(const char *addr, unsigned short int port)
{
	struct sockaddr_in server;
	int ret, sock;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		return -1;

	bzero((char *)&server, sizeof(server));
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
	close(sock);
}

