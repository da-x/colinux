/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_MESSAGES_H__
#define __CO_KERNEL_MESSAGES_H__

#include "list.h"
#include "queue.h"
#include "common.h"

#define CO_QUEUE_COUNT_LIMIT_BEFORE_SLEEP 1024

typedef struct co_message_queue_item {
	co_message_t *message;
} co_message_queue_item_t;

typedef char co_module_name_t[0x20];

extern co_rc_t co_message_dup_to_queue(co_message_t *message, co_queue_t *queue);
extern co_rc_t co_message_mov_to_queue(co_message_t *message, co_queue_t *queue);

extern char *co_module_repr(co_module_t module, co_module_name_t *str);

#endif
