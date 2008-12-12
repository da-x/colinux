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

#ifdef COLINUX_DEBUG
void co_debug_startup(void)
{
	static ULONG debug = 0;
	static RTL_QUERY_REGISTRY_TABLE query[2] = {{
		.Flags = RTL_QUERY_REGISTRY_REQUIRED
		       | RTL_QUERY_REGISTRY_DIRECT
		       | RTL_QUERY_REGISTRY_NOEXPAND,
		.Name = L"Debug",
		.EntryContext = &debug,
		.DefaultType = REG_DWORD,
		}};

	/* Enable DbgPrint, to see all the co_debug on loading driver. */
	if (RtlQueryRegistryValues(
			RTL_REGISTRY_ABSOLUTE,
			L"\\Registry\\MACHINE\\SOFTWARE\\coLinux",
			query, NULL, NULL) == STATUS_SUCCESS)
		co_global_debug_levels.misc_level = debug;
}

void co_debug_system(const char *fmt, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, fmt);
	co_vsnprintf(buf, sizeof(buf), fmt, ap);
	DbgPrint("%s\n", buf);
	va_end(ap);
}

void co_debug_level_system(const char *module, co_debug_facility_t facility, int level,
		    const char *filename, int line, const char *func, const char *text)
{
	/* Debug output to Dbgview.exe (www.sysinternals.com) */
        DbgPrint("[m:%s f:%d l:%d %s:%d:%s] %s\n", module, facility, level, filename, line, func, text);
}
#endif
