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

static void report_status(const char *event, co_manager_debug_t *debug)
{
	co_debug_system("colinux-debug: %s: sections: %d, total: %d, filled: %d\n",
			event, debug->sections_count, debug->sections_total_size,
			debug->sections_total_filled);
}

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
	section->peak_size = section->buffer_size;

	*section_out = section;
	
	return CO_RC(OK);
}

static void resize_section(co_manager_debug_t *debug, co_debug_section_t *section)
{
	long new_size = 0, old_size = 0;
	char *new_buffer, *old_buffer;

	if (section->filled >= section->buffer_size / 2) {
		/* 
		 * If the buffer is half full, increase it's size by a factor
		 * of two. This reduces the chance of losing logs due to write 
		 * bursts but keeps the buffers short.
		 */

		new_size = section->buffer_size * 2;
		if (new_size > CO_DEBUG_SECTION_BUFFER_MAX_SIZE)
			return;
	} else if (section->filled < CO_DEBUG_SECTION_BUFFER_START_SIZE) {
		/*
		 * Reduce to half the size of the peak size if the buffer is 
		 * quite empty.
		 */
		new_size = section->peak_size / 2;
	}

	if (new_size == 0)
		return;

	old_size = section->buffer_size;
	if (old_size != new_size)
		return;

	new_buffer = co_os_malloc(new_size);
	if (!new_buffer)
		return;

	old_buffer = section->buffer;
	co_memcpy(new_buffer, old_buffer, section->filled);
	
	debug->sections_total_size += new_size - old_size;
	section->buffer_size = new_size;
	section->buffer = new_buffer;

	if (section->peak_size < new_size)
		section->peak_size = new_size;

	report_status("section resize", debug);
	co_os_free(old_buffer);
}

static co_rc_t append_to_buffer(co_manager_debug_t *debug, 
				co_debug_section_t *section, 
				const char *buf, unsigned long size)
{
	co_rc_t rc = CO_RC(OK);

	resize_section(debug, section);

	/* Check if there's not enough space left in the buffer */
	if (section->buffer_size - section->filled < size) {
		rc = CO_RC(ERROR);
	} else {
		co_memcpy(&section->buffer[section->filled], buf, size);
		section->filled += size;
		debug->sections_total_filled += size;
	}

	return rc;
}

static void get_section(struct co_debug_section *section)
{
	section->refcount++;
	co_os_mutex_acquire(section->mutex);
}

static bool_t put_section(co_manager_debug_t *debug, 
			  struct co_debug_section *section)
{
	section->refcount--;
	if (section->refcount == 0) {
		debug->sections_count--;
		debug->sections_total_size -= section->buffer_size;
		co_os_mutex_release(section->mutex);
		co_os_mutex_destroy(section->mutex);
		co_os_free(section->buffer);
		co_os_free(section);
		report_status("section release", debug);
		return PFALSE;
	}
	co_os_mutex_release(section->mutex);
	return PTRUE;
}

co_rc_t co_debug_write(co_manager_debug_t *debug, 
		       struct co_debug_section **section_ptr,
		       const char *buf, long size)
{
	co_rc_t rc;
	co_debug_section_t *section;

	if (debug->sections_total_filled > CO_DEBUG_MAX_FILL)
		return CO_RC(OK);

	if (debug->freeing)
		return CO_RC(OK);

	section = *section_ptr;
	if (!section) {
		rc = create_section(&section);
		if (!CO_OK(rc)) 
			goto out;

		debug->sections_count++;
		debug->sections_total_size += section->buffer_size;

		co_os_mutex_acquire(debug->mutex);
		co_list_add_tail(&section->node, &debug->sections);
		co_os_mutex_release(debug->mutex);

		report_status("section created", debug);
		get_section(section);

		*section_ptr = section;
		section->refcount++;
	} else {
		get_section(section);
	}

	rc = append_to_buffer(debug, section, buf, size);

	if (!put_section(debug, section))
		*section_ptr = NULL;

	if (CO_OK(rc)) {
		co_os_wait_wakeup(debug->read_wait);
	}
	
out:
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
	long user_filled = 0;

	co_os_wait_sleep(debug->read_wait);
	co_os_mutex_acquire(debug->mutex);
	
	co_list_each_entry_safe(section, section_new, &debug->sections, node) {
		bool_t saved = PFALSE;
		get_section(section);

		if (!(size - user_filled < section->filled)) {
			rc = co_copy_to_user(&buf[user_filled], section->buffer, section->filled);
			if (CO_OK(rc)) {
				user_filled += section->filled;
				debug->sections_total_filled -= section->filled;
				section->filled = 0;
				saved = PTRUE;
			}
		}

		if (saved  &&  section->folded) {
			co_list_del(&section->node);
			section->refcount--;
			section->folded = PFALSE;
		}

		if (!section->folded)
			resize_section(debug, section);

		put_section(debug, section);
	}

	co_os_mutex_release(debug->mutex);

	*read_size = user_filled;

	return CO_RC(OK);
}


co_rc_t co_debug_fold(co_manager_debug_t *debug, co_debug_section_t *section)
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
		
		debug->sections_total_size -= section->buffer_size;
		section->buffer = shrunk_buffer;
		section->buffer_size = section->filled;
		debug->sections_total_size += section->buffer_size;
	}

	section->folded = PTRUE;

	put_section(debug, section);
	co_os_mutex_release(debug->mutex);

	return rc;
}

co_rc_t co_debug_init(co_manager_debug_t *debug)
{
	co_rc_t rc;

	debug->freeing = PFALSE;
	debug->section = NULL;
	debug->sections_total_filled = 0;
	debug->sections_total_size = 0;

	co_list_init(&debug->sections);

	rc = co_os_mutex_create(&debug->mutex);
	if (!CO_OK(rc))
		goto out;

	rc = co_os_wait_create(&debug->read_wait);
	if (!CO_OK(rc)) {
		co_os_mutex_destroy(debug->mutex);
		goto out;
	}

	/* co_debug_write_str(debug, &debug->section, "driver logging started\n"); */
	debug->ready = PTRUE;

out:
	return rc;
}

co_rc_t co_debug_free(co_manager_debug_t *debug)
{
	co_debug_section_t *section, *section_new;

	report_status("freeing all sections", debug);
	debug->ready = PFALSE;
	debug->freeing = PTRUE;

	co_list_each_entry_safe(section, section_new, &debug->sections, node) {
		get_section(section);
		co_list_del(&section->node);
		section->refcount--;
		put_section(debug, section);
	}

	co_os_wait_destroy(debug->read_wait);
	co_os_mutex_destroy(debug->mutex);
	report_status("done freeing", debug);
		
	return CO_RC(OK);
}

void co_debug_buf(const char *buf, long size)
{
	if (!co_global_manager) {
		return;
	}

	if (!co_global_manager->debug.ready) {
		return;
	}

	co_debug_write(&co_global_manager->debug, &co_global_manager->debug.section, buf, size);
}

