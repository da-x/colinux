/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include "misc.h"

/*
 * Currently there's a bug on SMP that prevents coLinux from
 * running stable unless it is pinned to the first processor.
 */
void co_winnt_affinity_workaround(void)
{
	SetProcessAffinityMask(GetCurrentProcess(), 1);
}

bool_t co_winnt_is_winxp_or_better(void)
{
	bool_t bRetVal = PFALSE;
	OSVERSIONINFO info = { sizeof(OSVERSIONINFO) };
	if (GetVersionEx(&info)) {
		if (info.dwMajorVersion > 5)
			bRetVal = PTRUE;
		else if (info.dwMajorVersion == 5 && info.dwMinorVersion > 0)
			bRetVal = PTRUE;
	}

	return bRetVal;
}
