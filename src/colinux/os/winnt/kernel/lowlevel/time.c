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

#include <colinux/os/kernel/time.h>

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

/*
 * Return 2^32 * (1 / (TSC clocks per usec)) for do_fast_gettimeoffset().
 */
unsigned long co_os_get_high_prec_quotient()
{
	LARGE_INTEGER PerformanceFrequency;
	unsigned long result;

	/* 
	 * HECK, it doesn't work, obviously.
	 */

	KeQueryPerformanceCounter(&PerformanceFrequency);

	result = (0x100000000LL * 1000000LL) / (PerformanceFrequency.QuadPart);

	co_debug("co_os_get_high_prec_time: %d\n", result);

	return result;
}
