/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "messages.h"

#include <memory.h>
#include <colinux/os/alloc.h>

void co_message_switch_init(co_message_switch_t *ms)
{
	co_list_init(&ms->list);
}

co_rc_t co_message_switch_set_rule(co_message_switch_t *ms, co_module_t destination,
				   co_switch_rule_func_t func, void *data)
{
	co_switch_rule_t *rule;

	rule = (co_switch_rule_t *)co_os_malloc(sizeof(*rule));
	if (rule == NULL)
		return CO_RC(ERROR);

	rule->destination = destination;
	rule->data = data;
	rule->func = func;

	co_list_add_head(&rule->node, &ms->list);

	return CO_RC_OK;
}

co_rc_t co_message_switch_cb_add_to_queue(void *data, co_message_t *message)
{
	co_queue_t *queue = (typeof(queue))data;
	co_message_queue_item_t *queue_item = NULL;
	co_rc_t rc = CO_RC_OK;

	rc = co_queue_malloc(queue, sizeof(co_message_queue_item_t), (void **)&queue_item);
	if (!CO_OK(rc))
		return rc;

	queue_item->message = message;

	co_queue_add_head(queue, queue_item);
	
	return CO_RC_OK;
}


co_rc_t co_message_switch_set_rule_queue(co_message_switch_t *ms, co_module_t destination, 
					 co_queue_t *queue)
{
	return co_message_switch_set_rule(ms, destination, 
					  co_message_switch_cb_add_to_queue,
					  queue);
}


co_rc_t co_message_switch_message(co_message_switch_t *ms, co_message_t *message)
{
	co_switch_rule_t *rule = NULL;

	co_list_each_entry(rule, &ms->list, node) {
		if (message->to == rule->destination) {
			return rule->func(rule->data, message);
		}
	}

	co_debug("switch_message: freed message %x (no handler)\n", message);
	
	co_os_free(message);

	return CO_RC(OK);
}

void co_message_switch_free(co_message_switch_t *ms)
{
	
}

co_rc_t co_message_dup(co_message_t *message, co_message_t **dup_message_out)
{
	unsigned long size; 
	co_message_t *allocated;
	
	size = sizeof(*message) + message->size;
	
	allocated = (co_message_t *)co_os_malloc(size);
	if (allocated == NULL)
		return CO_RC(ERROR);

	memcpy(allocated, message, size);

	*dup_message_out = allocated;
	
	return CO_RC(OK);
}


co_rc_t co_message_switch_dup_message(co_message_switch_t *ms, co_message_t *message)
{
	co_rc_t rc;
	co_message_t *copy;

	rc = co_message_dup(message, &copy);
	if (!CO_OK(rc))			
		return rc;
	
	return co_message_switch_message(ms, copy);
}
