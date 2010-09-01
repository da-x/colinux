#include "time.h"

#define DELTA (134774LL*86400)
#define SCALE (10000000LL)

unsigned long windows_time_to_unix_time(LARGE_INTEGER time)
{
	/*
	 * Windows' system time is a count of 100-nanosecond intervals since January 1, 1601.
	 * System time is typically updated approximately every ten milliseconds. This value
	 * is computed for the GMT time zone.
	 */

	/*
	 * Convert to seconds.
	 */
	time.QuadPart = time.QuadPart / SCALE;

	/*
	 * Shift to 1970. 134774*86400 is the number of seconds between January 1, 1601 and
	 * January 1, 1970.
	 */
	time.QuadPart -= DELTA;

	return time.QuadPart;
}

LARGE_INTEGER unix_time_to_windows_time(unsigned long time)
{
	LARGE_INTEGER ret_time;

	ret_time.QuadPart = time;
	ret_time.QuadPart += DELTA;
	ret_time.QuadPart *= SCALE;

	return ret_time;
}

