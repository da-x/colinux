/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_QUEUE_H__
#define __CO_KERNEL_QUEUE_H__

#include "list.h"
#include "common.h"

typedef struct co_queue_item {
	co_list_t node;
	unsigned char data[];
} co_queue_item_t;

typedef struct co_queue {
	co_list_t head;
	long items_count;
} co_queue_t;

/**
 * init a user-allocated queue.
 */
extern co_rc_t co_queue_init(co_queue_t *queue);

/**
 * free all the queue items (allocated using co_queue_malloc*()).
 */
extern co_rc_t co_queue_flush(co_queue_t *queue);

/**
 * return the number of items in the queue.
 */
static inline unsigned long co_queue_size(co_queue_t *queue)
{ return queue->items_count; }


/**
 * add an item (allocated by co_queue_malloc*()) to the queue, as
 * the head position.
 */
extern void co_queue_add_head(co_queue_t *queue, void *ptr);

/**
 * add an item (allocated by co_queue_malloc*()) to the queue, as
 * the tail position.
 */
extern void co_queue_add_tail(co_queue_t *queue, void *ptr);

/**
 * get the item at the tail but do not remove it.
 */
extern co_rc_t co_queue_get_tail(co_queue_t *queue, void **ptr);

/**
 * get the previous item in the queue (or NULL if we reached the head)
 */
extern co_rc_t co_queue_get_prev(co_queue_t *queue, void **ptr);

/**
 * removes an item from the tail and returns a pointer to it.
 */
extern co_rc_t co_queue_pop_tail(co_queue_t *queue, void **ptr);

/**
 * only peeks at an item from the tail and returns a pointer to it.
*/
extern co_rc_t co_queue_peek_tail(co_queue_t *queue, void **ptr);

/**
 * frees a buffer allocated by co_queue_malloc*().
 */
extern co_rc_t co_queue_free(co_queue_t *queue, void *ptr);

/**
 * allocate a queue item.
 */
extern co_rc_t co_queue_malloc(co_queue_t *queue, long bytes, void **ptr);

/**
 * allocate a queue item as a copy of a given buffer.
 */
extern co_rc_t co_queue_malloc_copy(co_queue_t *queue, void *fromptr, long bytes, void **ptr);

#endif
