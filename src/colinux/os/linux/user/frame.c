/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "frame.h"

#include <colinux/os/alloc.h>

co_rc_t co_os_frame_send(int sock, char *data, unsigned long size)
{
	co_rc_t rc = CO_RC(OK);
	
	rc = co_os_sendn(sock, (char *)&size, sizeof(unsigned long));
	if (!CO_OK(rc))
		return rc;

	return co_os_sendn(sock, data, size);
}

co_rc_t co_os_frame_recv(co_os_frame_collector_t *frame_collector, 
			 int sock, char **data, unsigned long *size)
{
	co_rc_t rc = CO_RC(OK);

	*data = NULL;
	*size = 0;

	if (frame_collector->buffer == NULL) {
		rc = co_os_recv(sock, (char *)(&frame_collector->size), 
				sizeof(frame_collector->size), 
				&frame_collector->buffer_received);

		if (!CO_OK(rc))
			return rc;

		if (sizeof(frame_collector->size) != frame_collector->buffer_received) { 
			/* size not fully received yet, maybe next time */
			return CO_RC(OK);
		}

		if (frame_collector->size > frame_collector->max_buffer_size) {
			/* frame too large, something is fishy */
			return CO_RC(ERROR);
		}

		frame_collector->buffer = (char *)(co_os_malloc(frame_collector->size));
		if (frame_collector->buffer == NULL) 
			return CO_RC(ERROR);

		frame_collector->buffer_received = 0;
	}

	rc = co_os_recv(sock, frame_collector->buffer, 
			frame_collector->size, &frame_collector->buffer_received);

	if (!CO_OK(rc))
		return rc;
	
	if (frame_collector->size != frame_collector->buffer_received) { 
		/* size not fully received yet, maybe next time */
		return CO_RC(OK);
	}
	
	*data = frame_collector->buffer;
	*size = frame_collector->size;

	frame_collector->buffer = NULL;
	frame_collector->buffer_received = 0;
	frame_collector->size = 0;

	return CO_RC(OK);
}
