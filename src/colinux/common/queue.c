/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/* OS independent generic queues */

#include <colinux/os/current/memory.h>
#include <colinux/os/alloc.h>

#include "queue.h"

co_rc_t co_queue_init(co_queue_t *queue)
{
	queue->items_count = 0;

	co_list_init(&queue->head);

	return CO_RC(OK);
}

co_rc_t co_queue_free(co_queue_t *queue, void *ptr)
{
	co_os_free(((char *)ptr) - sizeof(co_queue_item_t));

	return CO_RC(OK);
}

co_rc_t co_queue_malloc(co_queue_t *queue, long bytes, void **ptr)
{
	co_queue_item_t *item;

	item = co_os_malloc(bytes + sizeof(co_queue_item_t));
	if (item == NULL)
		return CO_RC(OUT_OF_MEMORY);

	*ptr = (void *)(&item->data);

	return CO_RC(OK);
}

co_rc_t co_queue_malloc_copy(co_queue_t *queue, void *fromptr, long bytes, void **ptr)
{
	co_rc_t rc;

	rc = co_queue_malloc(queue, bytes, ptr);
	if (!CO_OK(rc))
		return rc;

	memcpy(*ptr, fromptr, bytes);

	return rc;
}

co_rc_t co_queue_flush(co_queue_t *queue)
{
	co_rc_t rc = CO_RC(OK);

	while (1) {
		void *ptr;

		rc = co_queue_pop_tail(queue, &ptr);
		if (!CO_OK(rc))
			break;

		co_queue_free(queue, ptr);
	}

	return rc;
}

void co_queue_add_head(co_queue_t *queue, void *ptr)
{
	co_queue_item_t *item;

	item = (co_queue_item_t *)(((char *)ptr) - sizeof(co_queue_item_t));

	co_list_add_head(&item->node, &queue->head);

	queue->items_count += 1;
}

void co_queue_add_tail(co_queue_t *queue, void *ptr)
{
	co_queue_item_t *item;

	item = (co_queue_item_t *)(((char *)ptr) - sizeof(co_queue_item_t));

	co_list_add_tail(&item->node, &queue->head);

	queue->items_count += 1;
}

co_rc_t co_queue_get_tail(co_queue_t *queue, void **ptr)
{
	co_list_t *item;
	co_queue_item_t *queue_item;

	if (queue->items_count == 0)
		return CO_RC(ERROR);

	item = queue->head.prev;
	queue_item = co_list_entry(item, co_queue_item_t, node);
	*ptr = (void *)(&queue_item->data);

	return CO_RC(OK);
}

co_rc_t co_queue_get_prev(co_queue_t *queue, void **ptr)
{
	co_list_t *node = ((co_list_t *)((char *)(*ptr) - sizeof(co_list_t)));
	co_list_t *prev = node->prev;
	co_queue_item_t *queue_item;

	if (prev == &queue->head)
		return CO_RC(ERROR);

	queue_item = co_list_entry(prev, co_queue_item_t, node);
	*ptr = (void *)(&queue_item->data);

	return CO_RC(OK);
}

co_rc_t co_queue_pop_tail(co_queue_t *queue, void **ptr)
{
	co_list_t *item;
	co_queue_item_t *queue_item;

	if (queue->items_count == 0)
		return CO_RC(ERROR);

	queue->items_count -= 1;

	item = queue->head.prev;

	co_list_del(item);

	queue_item = co_list_entry(item, co_queue_item_t, node);

	*ptr = (void *)(&queue_item->data);

	return CO_RC(OK);
}

co_rc_t co_queue_peek_tail(co_queue_t *queue, void **ptr)
{
	co_list_t *item;
	co_queue_item_t *queue_item;

	if (queue->items_count == 0)
		return CO_RC(ERROR);

	item = queue->head.prev;

	queue_item = co_list_entry(item, co_queue_item_t, node);

	*ptr = (void *)(&queue_item->data);

	return CO_RC(OK);
}
