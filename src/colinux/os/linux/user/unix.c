/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "unix.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

co_rc_t co_os_set_blocking(int sock, bool_t enable)
{
	int ret;

	if (enable) {
		ret = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) & (~(O_NONBLOCK)));
	} else {
		ret = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	}

	if (ret == -1)
		return CO_RC(ERROR);
	return CO_RC(OK);
}

bool_t co_os_get_blocking(int sock)
{
	if (fcntl(sock, F_GETFL, 0) & O_NONBLOCK)
		return PTRUE;

	return PFALSE;
}

co_rc_t co_os_sendn(int sock, char *data, unsigned long size)
{
	co_rc_t rc = CO_RC(OK);
	bool_t blocking;

	if (size == 0)
		return CO_RC(OK);

	blocking = co_os_get_blocking(sock);
	co_os_set_blocking(sock, PTRUE);

	do {
		int sent = write(sock, data, size);
		if (sent < 0) {
			rc = CO_RC(ERROR);
			break;
		}
		if (sent == 0)
			break;

		size -= sent;
		data += sent;
	} while (size > 0);

	co_os_set_blocking(sock, blocking);

	return rc;
}

co_rc_t co_os_recv(int sock, char *data, unsigned long size, unsigned long *current_size)
{
	int received;
	if (size == *current_size)
		return CO_RC(OK);

	received = recv(sock, &data[*current_size], size - *current_size, 0);
	if (received < 0) {
		if (errno == EAGAIN) {
			return CO_RC(OK);
		}
		return CO_RC(ERROR);
	}

	*current_size += received;
	return CO_RC(OK);
}

