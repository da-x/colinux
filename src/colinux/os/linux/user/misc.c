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

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>

co_rc_t co_os_get_physical_ram_size(unsigned long *mem_size)
{
	struct stat statf;
	int ret;

	ret = stat("/proc/kcore", &statf);
	if (ret == -1)
		return CO_RC(ERROR);

	*mem_size = statf.st_size;

	/* Round by 16 MB */
	*mem_size = 0xFF000000 & ((*mem_size) + 0xFFFFFF);

	co_debug("Memory size: %d\n", *mem_size);

	return CO_RC(OK);
}

void co_terminal_print(const char *format, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	printf("%s", buf);
}

double co_os_timer_highres()
{
        struct timeval tv;

        gettimeofday(&tv, NULL);

        return ((double)tv.tv_sec) + ((double)tv.tv_usec)/1000000;
}
