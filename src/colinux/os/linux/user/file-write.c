/*
 * This source code is a part of coLinux source package.
 *
 * Henry Nestler, 2007 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <colinux/os/user/file.h>

co_rc_t co_os_file_write(co_pathname_t pathname, void *buf, unsigned long size)
{
	int fd, wr;

	fd = open((char *)pathname, O_WRONLY | O_CREAT | O_TRUNC,
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1)
		return CO_RC(ERROR);

	wr = write(fd, buf, size);
	close(fd);

	if (wr != size)
		return CO_RC(ERROR);

	return CO_RC(OK);
}
