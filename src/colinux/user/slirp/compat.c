#if COLINUX_ARCH == win32

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

#include <sys/time.h>
#include <time.h>

int co_gettimeofday(struct timeval *tv, int val)
{
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	tv->tv_usec = ((((ULARGE_INTEGER *)&ft)->QuadPart % 10000000)/10);
	tv->tv_sec = (((ULARGE_INTEGER *)&ft)->QuadPart / 10000000) % 86400;

	return 0;
}

/*
 * Thanks to John Smith (assemblywizard10 at yahoo.com) 
 */

void co_inet_aton(char* string, struct in_addr *out)
{
	char val[4][4];
	int c1, c2, i;
	struct _addr
	{
		union
		{
			unsigned long address;
			struct
			{
				unsigned char b[4];
			} b;
		} a;
	} addr;

	c1 = c2 = 0;
	memset(val, 0, sizeof(val));
	while(*string != '\0')
	{
		if(*string != '.')
			val[c1][c2++] = *string++;
		else
		{
			c2 = 0;
			c1++;
			string++;
		}
	}
	for(i = 0; i < 4; i++)
		addr.a.b.b[i] = atoi(val[i]);

	out->s_addr = addr.a.address;
}

extern int co_ioctl(int d, int request, ...)
{
	/* TODO */
	return 0;
}

#endif
