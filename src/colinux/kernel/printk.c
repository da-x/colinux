
#include <stdio.h>
#include <string.h>
#include "printk.h"

#include <linux/cooperative.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/common/libc.h>

void printk(co_monitor_t *cmon, char *fmt, ...) {
	co_message_t *co_message;
	char line[200];
	va_list ap;
	int len;

	va_start(ap,fmt);
	vsnprintf(line,200,fmt,ap);
	va_end(ap);
	len = strlen(line)+1;

	co_message = co_os_malloc(sizeof(*co_message) + len);
	if (co_message) {
		co_message->from = CO_MODULE_LINUX;
		co_message->to = CO_MODULE_PRINTK;
		co_message->priority = CO_PRIORITY_DISCARDABLE;
		co_message->type = CO_MESSAGE_TYPE_STRING;
		co_message->size = len;
		co_memcpy(co_message->data, line, len);
		incoming_message(cmon, co_message);
		co_os_free(co_message);
	}
}
