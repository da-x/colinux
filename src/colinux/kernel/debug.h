/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_DEBUG_H__
#define __COLINUX_KERNEL_DEBUG_H__

#include <colinux/common/common.h>
#include <colinux/common/list.h>
#include <colinux/os/kernel/mutex.h>
#include <colinux/os/kernel/wait.h>

struct co_manager;

#define CO_DEBUG_MAX_FILL                    (0x100000)
#define CO_DEBUG_SECTION_BUFFER_MAX_SIZE     (0x10000)
#define CO_DEBUG_SECTION_BUFFER_START_SIZE   (0x1000)

typedef struct co_debug_section {
	co_list_t node;
	char *buffer;
	long buffer_size;
	long peak_size;
	long filled;
	co_os_mutex_t mutex;
	int refcount;
	long max_size;
	bool_t folded;
} co_debug_section_t;

typedef struct co_manager_debug {
	bool_t ready;
	co_list_t sections;
	co_os_mutex_t mutex;
	co_debug_section_t *section;
	co_os_wait_t read_wait;
	bool_t freeing;
	int sections_count;
	long sections_total_size;
	long sections_total_filled;
} co_manager_debug_t;

struct co_manager_per_fd_state;

extern co_rc_t co_debug_init(co_manager_debug_t *manager);

typedef struct co_debug_write_vector {
	int vec_size;
	long size;
	union {
		const char *ptr;
		struct co_debug_write_vector *vec;
	};
} co_debug_write_vector_t;

extern co_rc_t co_debug_write_log(co_manager_debug_t *debug,
				  struct co_debug_section **section_ptr,
				  co_debug_write_vector_t *vec, int vec_size);

extern co_rc_t co_debug_read(co_manager_debug_t *manager,
			     char *buf, unsigned long size,
			     unsigned long *read_size);

extern co_rc_t co_debug_fold(co_manager_debug_t *manager,
			     co_debug_section_t *section);

extern co_rc_t co_debug_free(co_manager_debug_t *manager);

#endif
