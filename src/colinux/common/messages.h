/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_MESSAGES_H__
#define __CO_KERNEL_MESSAGES_H__

#include "list.h"
#include "queue.h"
#include "common.h"

typedef co_rc_t (*co_switch_rule_func_t)(void *data, co_message_t *message);

typedef struct co_switch_rule {
	co_module_t destination;
	co_switch_rule_func_t func;
	void *data;
	co_list_t node;
} co_switch_rule_t;

typedef struct co_message_queue_item {
	co_message_t *message;
} co_message_queue_item_t;

typedef struct co_message_switch {
	co_list_t list;
} co_message_switch_t;

extern void co_message_switch_init(co_message_switch_t *ms);
extern co_rc_t co_message_switch_set_rule(co_message_switch_t *ms, co_module_t destination,
					  co_switch_rule_func_t func, void *data);
extern co_rc_t co_message_switch_set_rule_queue(co_message_switch_t *ms, co_module_t destination, co_queue_t *queue);
extern co_rc_t co_message_switch_message(co_message_switch_t *ms, co_message_t *message);
extern co_rc_t co_message_switch_dup_message(co_message_switch_t *ms, co_message_t *message);
extern void co_message_switch_free(co_message_switch_t *ms);
extern co_rc_t co_message_dup(co_message_t *message, co_message_t **dup_message_out);

#endif
