/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "../ddk.h"

#include <colinux/common/common.h>
#include <colinux/os/kernel/time.h>
#include <colinux/os/timer.h>

unsigned long co_os_get_time()
{
	LARGE_INTEGER CurrentTime;

	/* 
	 * Windows' system time is a count of 100-nanosecond intervals since January 1, 1601. 
	 * System time is typically updated approximately every ten milliseconds. This value 
	 * is computed for the GMT time zone. 
	 */
 
	KeQuerySystemTime(&CurrentTime);
	
	/* 
	 * Convert to seconds.
	 */
	CurrentTime.QuadPart = CurrentTime.QuadPart / 10000000LL;

	/*
	 * Shift to 1970. 134774*86400 is the number of seconds between January 1, 1601 and
	 * January 1, 1970.
	 */  
	CurrentTime.QuadPart -= 134774LL*86400; 

	return CurrentTime.QuadPart;
}

void co_os_get_timestamp(co_timestamp_t *dts)
{
	LARGE_INTEGER PerformanceFrequency;
	LARGE_INTEGER PerformanceCounter;

	PerformanceCounter = KeQueryPerformanceCounter(&PerformanceFrequency);

	dts->high = PerformanceCounter.HighPart;
	dts->low = PerformanceCounter.LowPart;
}

void co_os_get_timestamp_freq(co_timestamp_t *dts)
{
	LARGE_INTEGER PerformanceFrequency;

	KeQueryPerformanceCounter(&PerformanceFrequency);

	dts->high = PerformanceFrequency.HighPart;
	dts->low = PerformanceFrequency.LowPart;
}

