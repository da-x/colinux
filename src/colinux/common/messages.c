/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/os/current/memory.h>
#include <colinux/os/alloc.h>

#include "messages.h"

static
co_rc_t co_message_dup(co_message_t *message, co_message_t **dup_message_out)
{
	unsigned long size; 
	co_message_t *allocated;
	
	size = sizeof(*message) + message->size;
	
	allocated = co_os_malloc(size);
	if (allocated == NULL)
		return CO_RC(OUT_OF_MEMORY);

	memcpy(allocated, message, size);

	*dup_message_out = allocated;
	
	return CO_RC(OK);
}

co_rc_t co_message_dup_to_queue(co_message_t *message, co_queue_t *queue)
{
	co_message_t *dup_message;
	co_rc_t rc;
	
	rc = co_message_dup(message, &dup_message);
	if (CO_OK(rc)) {
		co_message_queue_item_t *queue_item = NULL;
		
		rc = co_queue_malloc(queue, sizeof(co_message_queue_item_t), (void **)&queue_item);
		if (!CO_OK(rc)) {
			co_os_free(dup_message);
		} else {			
			queue_item->message = dup_message;
			co_queue_add_head(queue, queue_item);
		}
	}

	return rc;
}

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
