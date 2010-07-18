/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/file.h>

co_rc_t co_os_file_load(co_pathname_t pathname, char **out_buf, unsigned long *out_size, unsigned long max_size)
{
	int fd, ret;
	char *buf;
	struct stat st;

	fd = open((char *)pathname, O_RDONLY);
	if (fd == -1)
		return CO_RC(ERROR);

	ret = fstat(fd, &st);
	if (ret == -1) {
		close(fd);
		return CO_RC(ERROR);
	}

	if (max_size && st.st_size > max_size)
		st.st_size = max_size;

	buf = (char *)malloc(st.st_size);
	if (buf == NULL)  {
		close(fd);
		return CO_RC(OUT_OF_MEMORY);
	}

	if (read(fd, buf, st.st_size) != st.st_size) {
		free(buf);
		close(fd);
		return CO_RC(ERROR);
	}

	*out_buf = buf;
	*out_size = st.st_size;

	close(fd);
	return CO_RC(OK);
}

void co_os_file_free(char *buf)
{
	free(buf);
}
