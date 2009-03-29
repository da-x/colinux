/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "messages.h"

char *co_module_repr(co_module_t module, co_module_name_t *str)
{
	switch (module) {
	case CO_MODULE_LINUX: co_snprintf((char *)str, sizeof(*str), "linux"); break;
	case CO_MODULE_MONITOR: co_snprintf((char *)str, sizeof(*str), "monitor"); break;
	case CO_MODULE_DAEMON: co_snprintf((char *)str, sizeof(*str), "daemon"); break;
	case CO_MODULE_IDLE: co_snprintf((char *)str, sizeof(*str), "idle"); break;
	case CO_MODULE_KERNEL_SWITCH: co_snprintf((char *)str, sizeof(*str), "kernel"); break;
	case CO_MODULE_USER_SWITCH: co_snprintf((char *)str, sizeof(*str), "user"); break;
	case CO_MODULE_CONSOLE: co_snprintf((char *)str, sizeof(*str), "console"); break;
	case CO_MODULE_PRINTK: co_snprintf((char *)str, sizeof(*str), "printk"); break;
	case CO_MODULE_CONET0...CO_MODULE_CONET_END - 1:
		co_snprintf((char *)str, sizeof(*str), "net%d", module - CO_MODULE_CONET0);
		break;
	case CO_MODULE_COSCSI0...CO_MODULE_COSCSI_END - 1:
		co_snprintf((char *)str, sizeof(*str), "scsi%d", module - CO_MODULE_COSCSI0);
		break;
	case CO_MODULE_COBD0...CO_MODULE_COBD_END - 1:
		co_snprintf((char *)str, sizeof(*str), "cobd%d", module - CO_MODULE_COBD0);
		break;
	case CO_MODULE_SERIAL0...CO_MODULE_SERIAL_END - 1:
		co_snprintf((char *)str, sizeof(*str), "serial%d", module - CO_MODULE_SERIAL0);
		break;
	default:
		co_snprintf((char *)str, sizeof(*str), "unknown<%d>", module);
	}

	return (char *)str;
}
