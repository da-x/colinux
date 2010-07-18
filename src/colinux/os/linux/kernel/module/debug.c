/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"
#include "manager.h"
#include <colinux/kernel/debug.h>

static int debug_misc;
module_param(debug_misc, int, 0);
MODULE_PARM_DESC(debug_misc, "Debug level for misc");

void co_debug_startup(void)
{
	co_global_debug_levels.misc_level = debug_misc;
	if (debug_misc)
		printk(KERN_INFO "coLinux: debug_misc %d\n", debug_misc);
}

void co_debug_system(const char *fmt, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, fmt);
	co_vsnprintf(buf, sizeof(buf), fmt, ap);
	printk(KERN_INFO "%s\n", buf);
	va_end(ap);
}

void co_debug_level_system(const char *module, co_debug_facility_t facility, int level,
		    const char *filename, int line, const char *func, const char *text)
{
	const char *fname;

	/* Strip full path from name */
	fname = strrchr(filename, '/');
	if (fname) {
		for (fname = fname-1;
		     fname > filename && *fname != '.' && *fname != '/';
		     fname--)
			;
		fname++;
	} else
		fname = filename;

	/* Debug output to Kernel log */
	printk(KERN_INFO "[m:%s f:%d l:%d %s:%d:%s] %s\n", module, facility, level, fname, line, func, text);
}
