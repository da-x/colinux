/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "messages.h"

#include <memory.h>
#include <colinux/os/alloc.h>

#define co_debug_lvl_route(msg, from, to, level, fmt, ...) do {	\
    co_module_name_t from_str;						\
    co_module_name_t to_str;						\
    									\
    co_debug_lvl(messages, level, msg " %s -> %s: " fmt,		\
		 co_module_repr(from, &from_str),			\
		 co_module_repr(to, &to_str),				\
		 ## __VA_ARGS__);					\
} while (0);

#define co_debug_lvl_message(message, level, fmt, ...)			\
	co_debug_lvl_route("route", message->from, message->to, level, fmt, ## __VA_ARGS__)

void co_message_switch_init(co_message_switch_t *ms, co_module_t switch_id)
{
	co_list_init(&ms->list);
	ms->switch_id = switch_id;
}

co_rc_t co_message_switch_set_rule(co_message_switch_t *ms, co_module_t destination,
				   co_switch_rule_func_t func, void *data)
{
	co_switch_rule_t *rule;
	co_module_name_t destination_str;

	rule = (co_switch_rule_t *)co_os_malloc(sizeof(*rule));
	if (rule == NULL)
		return CO_RC(ERROR);

	rule->type = CO_SWITCH_RULE_TYPE_CALLBACK;
	rule->destination = destination;
	rule->data = data;
	rule->func = func;

	co_debug_lvl(messages, 5, "setting callback rule for %s\n", co_module_repr(destination, &destination_str));

	co_list_add_head(&rule->node, &ms->list);

	return CO_RC_OK;
}

co_rc_t co_message_switch_set_rule_reroute(co_message_switch_t *ms, co_module_t destination, co_module_t new_destination)
{
	co_switch_rule_t *rule;

	rule = (co_switch_rule_t *)co_os_malloc(sizeof(*rule));
	if (rule == NULL)
		return CO_RC(ERROR);

	rule->type = CO_SWITCH_RULE_TYPE_REROUTE;
	rule->destination = destination;
	rule->reroute_destination = new_destination;

	co_debug_lvl_route("reroute rule", destination, new_destination, 11, "\n");

	co_list_add_head(&rule->node, &ms->list);

	return CO_RC_OK;
}

co_rc_t co_message_switch_cb_add_to_queue(void *data, co_message_t *message)
{
	co_queue_t *queue = (typeof(queue))data;
	co_message_queue_item_t *queue_item = NULL;
	co_rc_t rc = CO_RC_OK;

	rc = co_queue_malloc(queue, sizeof(co_message_queue_item_t), (void **)&queue_item);
	if (!CO_OK(rc)) {
		co_os_free(message);
		return rc;
	}

	co_debug_lvl_message(message, 11, "adding to queue %x\n", queue);

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


co_rc_t co_message_switch_get_rule(co_message_switch_t *ms, co_module_t destination,
				   co_switch_rule_t **out_rule)
{
	co_switch_rule_t *rule = NULL;

	co_list_each_entry(rule, &ms->list, node) {
		if (destination == rule->destination) {
			*out_rule = rule;
			return CO_RC(OK);
		}
	}
	
	return CO_RC(ERROR);
}

bool_t co_message_switch_rule_exists(co_message_switch_t *ms, co_module_t destination)
{
	co_switch_rule_t *rule = NULL;
	co_rc_t rc;
	
	rc = co_message_switch_get_rule(ms, destination, &rule);
	if (!CO_OK(rc))
		return PFALSE;

	return PTRUE;
}

co_rc_t co_message_switch_message(co_message_switch_t *ms, co_message_t *message)
{
	co_switch_rule_t *rule = NULL;
	co_rc_t rc = CO_RC(OK);

	if (message->to == ms->switch_id) {
		co_switch_message_t *switch_message;

		/* A message for me */
		if (message->size != sizeof(co_switch_message_t)) {
			co_debug_lvl_message(message, 4, "bad messasge size (%d != %d)", message->size,  sizeof(co_switch_message_t));
			goto out_free_error;
		}
		
		switch_message = (co_switch_message_t *)message->data;
		switch (switch_message->type) {
		case CO_SWITCH_MESSAGE_SET_REROUTE_RULE:
			rc = co_message_switch_set_rule_reroute(ms, switch_message->destination,
								switch_message->reroute_destination);
			break;
		case CO_SWITCH_MESSAGE_FREE_RULE:
			rc = co_message_switch_free_rule(ms, switch_message->destination);
			break;
		default:
			co_debug_lvl_message(message, 4, "bad switch message type (%d)\n", switch_message->type);
			goto out_free_error;
		}

		goto out_free;
	}
	
	rc = co_message_switch_get_rule(ms, message->to, &rule);
	if (CO_OK(rc)) {
		if (rule->type == CO_SWITCH_RULE_TYPE_REROUTE) {
			rc = co_message_switch_get_rule(ms, rule->reroute_destination, &rule);
			if (!CO_OK(rc))
				goto out_free_error;
		}

		if (rule->type == CO_SWITCH_RULE_TYPE_CALLBACK)
			return rule->func(rule->data, message);
	}

out_free_error:
	co_debug_lvl_message(message, 11, "freed message\n");

out_free:
	co_os_free(message);

	return rc;
}

co_rc_t co_message_switch_free_rule(co_message_switch_t *ms, co_module_t destination)
{
	co_switch_rule_t *rule = NULL;
	co_module_name_t destination_str;
	co_rc_t rc;

	rc = co_message_switch_get_rule(ms, destination, &rule);
	if (!CO_OK(rc))
		return rc;

	co_debug_lvl(messages, 5, "freeng rule for %s\n", co_module_repr(destination, &destination_str));

	co_list_del(&rule->node);
	co_os_free(rule);

	return CO_RC(OK);
}

void co_message_switch_free(co_message_switch_t *ms)
{
	co_switch_rule_t *rule;

	while (!co_list_empty(&ms->list)) {
		rule = co_list_entry(ms->list.next, co_switch_rule_t, node);

		co_list_del(&rule->node);	
		co_os_free(rule);
	}
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

co_rc_t co_message_switch_dup_message(co_message_switch_t *ms, co_message_t *message)
{
	co_rc_t rc;
	co_message_t *copy;

	rc = co_message_dup(message, &copy);
	if (!CO_OK(rc))			
		return rc;
	
	return co_message_switch_message(ms, copy);
}

co_rc_t co_message_write_queue(co_queue_t *queue, char *buffer, unsigned long size,
			       unsigned long *sent_count, unsigned long *sent_size)
{
	char *buffer_writeptr = buffer;
	unsigned long size_left = size;
	unsigned long queue_items;
	co_rc_t rc;

	queue_items = co_queue_size(queue);
	while (queue_items > 0) {
		co_message_queue_item_t *message_item;
		unsigned long total_size;
		co_message_t *message;
			
		rc = co_queue_pop_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			break;

		message = message_item->message;
		total_size = message->size + sizeof(*message);

		if (total_size > size_left) {
			co_queue_add_tail(queue, message_item);
			break;
		}				
			
		memcpy(buffer_writeptr, message, total_size);

		queue_items -= 1;
		size_left -= total_size;
		buffer_writeptr += total_size;
		*sent_count += 1;

		co_queue_free(queue,  message_item);
		co_os_free(message);
	}

	*sent_size = buffer_writeptr - buffer;

	return CO_RC(OK);
}

char *co_module_repr(co_module_t module, co_module_name_t *str)
{
	switch (module) {
	case CO_MODULE_LINUX: co_snprintf((char *)str, sizeof(*str), "monitor"); break;
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
