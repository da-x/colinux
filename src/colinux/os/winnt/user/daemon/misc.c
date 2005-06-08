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
	char *env = getenv("COLINUX_NO_SMP_WORKAROUND");

	if (env != NULL) {
		if (strcmp(env, "Y") == 0)
			return;
	}

	SetProcessAffinityMask(GetCurrentProcess(), 1);
}

