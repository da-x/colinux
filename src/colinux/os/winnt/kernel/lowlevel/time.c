/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#include <colinux/os/kernel/time.h>

unsigned long co_os_get_time()
{
	LARGE_INTEGER CurrentTime;
	unsigned long long days_between_1601_and_1970 = 0;
	unsigned long long days_between_1601_and_1968 = 0;

	/* 
	 * Windows' system time is a count of 100-nanosecond intervals since January 1, 1601. 
	 * System time is typically updated approximately every ten milliseconds. This value 
	 * is computed for the GMT time zone. 
	 */
 
	KeQuerySystemTime(&CurrentTime);
	
	days_between_1601_and_1968 = ((365*4+1)*92 - 365);
	days_between_1601_and_1970 = days_between_1601_and_1968 + 366 + 365;
	
	/* 
	 * Convert to seconds.
	 */
	CurrentTime.QuadPart = CurrentTime.QuadPart / 10000000LL;

	/*
	 * Shift to 1970.
	 */ 
	CurrentTime.QuadPart -= days_between_1601_and_1970*86400;

	return CurrentTime.QuadPart;
}
