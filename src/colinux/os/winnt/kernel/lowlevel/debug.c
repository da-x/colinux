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
#include "../manager.h"

#include <colinux/common/debug.h>

void co_debug_line(char *line)
{
	if (!co_global_manager) {
		DbgPrint("%s", line);
		return;
	}

	if (!co_global_manager->debug.ready) {
		DbgPrint("%s", line);
		return;
	}

	co_debug_write_str(&co_global_manager->debug, &co_global_manager->debug.section, line);
}
