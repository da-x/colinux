/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/* OS independent message queue */

#include <colinux/os/current/memory.h>
#include <colinux/os/alloc.h>

#include "messages.h"

static co_rc_t co_message_dup(co_message_t*  message,
                              co_message_t** dup_message_out)
{
	unsigned long size;
	co_message_t* allocated;

	size = sizeof(*message) + message->size;

	allocated = co_os_malloc(size);
	if (allocated == NULL)
		return CO_RC(OUT_OF_MEMORY);

	memcpy(allocated, message, size);

	*dup_message_out = allocated;

	return CO_RC(OK);
}

co_rc_t co_message_dup_to_queue(co_message_t* message, co_queue_t* queue)
{
	co_message_t* dup_message;
	co_rc_t	      rc;

	rc = co_message_dup(message, &dup_message);
	if (CO_OK(rc)) {
		co_message_queue_item_t* queue_item = NULL;

		rc = co_queue_malloc(queue, sizeof(co_message_queue_item_t), (void**)&queue_item);
		if (!CO_OK(rc)) {
			co_os_free(dup_message);
		} else {
			queue_item->message = dup_message;
			co_queue_add_head(queue, queue_item);
		}
	}

	return rc;
}

co_rc_t co_message_mov_to_queue(co_message_t* message, co_queue_t* queue)
{
	co_message_queue_item_t* queue_item = NULL;
	co_rc_t			 rc;

	rc = co_queue_malloc(queue, sizeof(co_message_queue_item_t), (void **)&queue_item);
	if (!CO_OK(rc))
		return rc;

	queue_item->message = message;
	co_queue_add_head(queue, queue_item);

	return CO_RC(OK);
}
