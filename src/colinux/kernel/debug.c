/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/os/alloc.h>
#include <colinux/os/kernel/user.h>
#include <colinux/common/libc.h>

#include "manager.h"
#include "debug.h"

static co_rc_t create_section(co_debug_section_t **section_out)
{
	co_debug_section_t *section;
	co_rc_t rc;

	section = (co_debug_section_t *)co_os_malloc(sizeof(co_debug_section_t));
	if (!section)
		return CO_RC(OUT_OF_MEMORY);
	
	section->buffer = co_os_malloc(CO_DEBUG_SECTION_BUFFER_START_SIZE);
	if (!section->buffer) {
		co_os_free(section);
		return CO_RC(OUT_OF_MEMORY);
	}

	rc = co_os_mutex_create(&section->mutex);
	if (!CO_OK(rc)) {
		co_os_free(section->buffer);
		co_os_free(section);
		return rc;
	}

	co_list_init(&section->node);
	section->buffer_size = CO_DEBUG_SECTION_BUFFER_START_SIZE;
	section->filled = 0;
	section->refcount = 0;
	section->folded = PFALSE;

	*section_out = section;
	
	return CO_RC(OK);
}


static co_rc_t append_to_buffer(co_debug_section_t *section, 
				const char *buf, unsigned long size)
{
	co_rc_t rc = CO_RC(OK);

	/* Check if there's not enough space left in the buffer */
	if (section->buffer_size - section->filled < size) {
		rc = CO_RC(ERROR);
	} else {
		co_memcpy(&section->buffer[section->filled], buf, size);
		section->filled += size;
	}

	return rc;
}

static void get_section(struct co_debug_section *section)
{
	section->refcount++;
	co_os_mutex_acquire(section->mutex);
}

static bool_t put_section(struct co_debug_section *section)
{
	section->refcount--;
	if (section->refcount == 0) {
		co_os_mutex_release(section->mutex);
		co_os_mutex_destroy(section->mutex);
		co_os_free(section->buffer);
		co_os_free(section);
		return PFALSE;
	}
	co_os_mutex_release(section->mutex);
	return PTRUE;
}

co_rc_t co_debug_write(co_manager_debug_t *debug, 
		       struct co_debug_section **section_ptr,
		       const char *buf, unsigned long size)
{
	co_rc_t rc;
	co_debug_section_t *section;

	if (debug->freeing)
		return CO_RC(OK);

	section = *section_ptr;
	if (!section) {
		rc = create_section(&section);
		if (!CO_OK(rc))
			return rc;

		co_os_mutex_acquire(debug->mutex);
		co_list_add_tail(&section->node, &debug->sections);
		co_os_mutex_release(debug->mutex);

		get_section(section);

		*section_ptr = section;
		section->refcount++;
	} else {
		get_section(section);
	}

	rc = append_to_buffer(section, buf, size);

	if (!put_section(section))
		*section_ptr = NULL;

	if (CO_OK(rc)) {
		co_os_wait_wakeup(debug->read_wait);
	}

	return rc;
}

co_rc_t co_debug_write_str(co_manager_debug_t *debug, 
			   struct co_debug_section **section_ptr,
			   const char *str)
{
	return co_debug_write(debug, section_ptr, str, strlen(str));
}

co_rc_t co_debug_read(co_manager_debug_t *debug, 
		      char *buf, unsigned long size, 
		      unsigned long *read_size)
{
	co_rc_t rc;
	co_debug_section_t *section, *section_new;
	unsigned long user_filled = 0;

	co_os_wait_sleep(debug->read_wait);
	co_os_mutex_acquire(debug->mutex);
	
	co_list_each_entry_safe(section, section_new, &debug->sections, node) {
		bool_t saved = PFALSE;
		get_section(section);

		if (!(size - user_filled < section->filled)) {
			rc = co_copy_to_user(&buf[user_filled], section->buffer, section->filled);
			if (CO_OK(rc)) {
				user_filled += section->filled;
				section->filled = 0;
				saved = PTRUE;
			}
		}

		if (saved  &&  section->folded) {
			co_list_del(&section->node);
			section->refcount--;
			section->folded = PFALSE;
		}

		put_section(section);
	}

	co_os_mutex_release(debug->mutex);

	*read_size = user_filled;

	return CO_RC(OK);
}


co_rc_t co_debug_fold(co_manager_debug_t *debug, 
		      co_debug_section_t *section)
{
	char *shrunk_buffer;
	co_rc_t rc = CO_RC(OK);

	co_os_mutex_acquire(debug->mutex);
	get_section(section);
	
	shrunk_buffer = co_os_malloc(section->filled + 1);
	if (!shrunk_buffer) {
		rc = CO_RC(OUT_OF_MEMORY);
	} else {
		co_memcpy(shrunk_buffer, section->buffer, section->filled);
		co_os_free(section->buffer);
		
		section->buffer = shrunk_buffer;
		section->buffer_size = section->filled;
	}

	section->folded = PTRUE;

	put_section(section);
	co_os_mutex_release(debug->mutex);

	return rc;
}

co_rc_t co_debug_init(co_manager_debug_t *debug)
{
	co_rc_t rc;

	debug->freeing = PFALSE;
	debug->section = NULL;
	co_list_init(&debug->sections);

	rc = co_os_mutex_create(&debug->mutex);
	if (!CO_OK(rc))
		return rc;

	rc = co_os_wait_create(&debug->read_wait);
	if (!CO_OK(rc)) {
		co_os_mutex_destroy(debug->mutex);
		return rc;
	}

	co_debug_write_str(debug, &debug->section, "driver logging started\n");
	debug->ready = PTRUE;

	return CO_RC(OK);
}

co_rc_t co_debug_free(co_manager_debug_t *debug)
{
	co_debug_section_t *section, *section_new;

	debug->ready = PFALSE;
	debug->freeing = PTRUE;

	co_list_each_entry_safe(section, section_new, &debug->sections, node) {
		get_section(section);
		co_list_del(&section->node);
		section->refcount--;
		put_section(section);
	}

	co_os_wait_destroy(debug->read_wait);
	co_os_mutex_destroy(debug->mutex);

	return CO_RC(OK);
}
