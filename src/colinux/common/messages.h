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

typedef co_rc_t (*co_switch_rule_func_t)(void *data, co_message_t *message);

typedef enum {
	CO_SWITCH_RULE_TYPE_CALLBACK,
	CO_SWITCH_RULE_TYPE_REROUTE,
} co_switch_rule_type;

typedef struct co_switch_rule {
	co_switch_rule_type type;
	co_module_t destination;
	co_list_t node;
	union {
		struct {
			co_switch_rule_func_t func;
			void *data;
		};
		co_module_t reroute_destination;
	};
} co_switch_rule_t;

typedef enum {
	CO_SWITCH_MESSAGE_SET_REROUTE_RULE,
	CO_SWITCH_MESSAGE_FREE_RULE,
} co_switch_message_type;

typedef struct co_switch_message {
	co_switch_message_type type;
	co_module_t destination;
	co_module_t reroute_destination;
} co_switch_message_t;

typedef struct co_message_queue_item {
	co_message_t *message;
} co_message_queue_item_t;

typedef struct co_message_switch {
	co_module_t switch_id;
	co_list_t list;
} co_message_switch_t;

typedef char co_module_name_t[0x20];

extern void co_message_switch_init(co_message_switch_t *ms, co_module_t switch_id);
extern co_rc_t co_message_switch_set_rule(co_message_switch_t *ms, co_module_t destination,
					  co_switch_rule_func_t func, void *data);
extern co_rc_t co_message_switch_set_rule_reroute(co_message_switch_t *ms, co_module_t destination, co_module_t new_destination);
extern co_rc_t co_message_switch_set_rule_queue(co_message_switch_t *ms, co_module_t destination, co_queue_t *queue);
extern co_rc_t co_message_switch_free_rule(co_message_switch_t *ms, co_module_t destination);
extern bool_t co_message_switch_rule_exists(co_message_switch_t *ms, co_module_t destination);
extern co_rc_t co_message_switch_message(co_message_switch_t *ms, co_message_t *message);
extern co_rc_t co_message_switch_dup_message(co_message_switch_t *ms, co_message_t *message);
extern void co_message_switch_free(co_message_switch_t *ms);
extern co_rc_t co_message_dup(co_message_t *message, co_message_t **dup_message_out);
extern co_rc_t co_message_dup_to_queue(co_message_t *message, co_queue_t *queue);
extern co_rc_t co_message_write_queue(co_queue_t *queue, char *buffer, unsigned long size,
				      unsigned long *sent_count, unsigned long *sent_size);

extern char *co_module_repr(co_module_t module, co_module_name_t *str);

#endif
