/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __CO_KERNEL_MONITOR_H__
#define __CO_KERNEL_MONITOR_H__

#include <colinux/common/queue.h>
#include <colinux/common/config.h>
#include <colinux/common/import.h>
#include <colinux/common/ioctl.h>
#include <colinux/common/console.h>
#include <colinux/common/messages.h>
#include <colinux/arch/current/mmu.h>
#include <colinux/os/kernel/mutex.h>
#include <colinux/os/timer.h>

struct co_monitor_device;
struct co_monitor;

typedef co_rc_t (*co_monitor_service_func_t)(struct co_monitor *cmon, 
					     struct co_monitor_device *device,
					     unsigned long *params);
typedef struct co_monitor_device {
	co_monitor_service_func_t service;
	unsigned long state;
	void *data;	
	co_queue_t out_queue;
	co_queue_t in_queue;
	co_os_mutex_t mutex;
} co_monitor_device_t;

typedef enum {
	CO_MONITOR_STATE_INITIALIZED,
	CO_MONITOR_STATE_RUNNING,
	CO_MONITOR_STATE_TERMINATED,
} co_monitor_state_t;

typedef enum {
	CO_MONITOR_CONSOLE_STATE_DETACHED,
	CO_MONITOR_CONSOLE_STATE_ATTACHED,
} co_monitor_console_state_t;

/*
 * We use the following struct for each coLinux system. 
 */
typedef struct co_monitor {
	/*
	 * Pointer back to the manager that controls us and
	 * our id in that manager.
	 */
	struct co_manager *manager; 
	co_id_t id;

	/*
	 * OS-dependant data:
	 */
	struct co_monitor_osdep *osdep; 

	/*
	 * State of monitor.
	 */ 
	co_monitor_state_t state;

	/*
	 * State of the connected console.
	 */
	co_monitor_console_state_t console_state;

	/*
	 * Configuration data.
	 */
	co_config_t config;
 
	/*
	 * The passage page
	 */
	struct co_arch_passage_page *passage_page;  /* The virtual address of the 
						       PP in the host */
	vm_ptr_t passage_page_vaddr;                /* The virtual address of the 
						       PP in Linux */

	/*
	 * Core stuff (Linux kernel image) 
	 */
	vm_ptr_t core_vaddr;             /* Where the core sits (the famous C0100000) */
	vm_ptr_t core_end;               /* Where the core ends */
	unsigned long core_pages;        /* number of pages our core takes */    
	co_symbols_import_t import;      /* Addresse of symbols in the kernel */

	/*
	 * Pseudo physical RAM
	 */
	unsigned long memory_size;      /* The size of Linux's pseudo physical RAM */
	unsigned long physical_frames;  /* The number of pages in that RAM */
	unsigned long end_physical;     /* In what virtual address the map of the 
					   pseudo physical RAM ends */
	co_pfn_t **pp_pfns;

	/* 
	 * Dynamic allocations in the host
	 */
	unsigned long blocks_allocated;

	/*
	 * Page global directory
	 */
	linux_pgd_t pgd;             /* Pointer to the physical address PGD of 
					Linux (to be put in CR3 eventually */
	co_pfn_t **pgd_pfns;

	/*
	 * Devices
	 */
	co_monitor_device_t devices[CO_DEVICES_TOTAL];
	bool_t timer_interrupt;

        /*
         * Timer
         */
        co_os_timer_t timer;

	/* 
	 * Block devices
	 */
	struct co_block_dev *block_devs[CO_MODULE_MAX_COBD];

	/*
	 * Message passing stuff
	 */ 
	co_message_switch_t message_switch;
	co_queue_t user_message_queue;
	co_queue_t linux_message_queue;
} co_monitor_t;

extern co_rc_t co_monitor_create(struct co_manager *manager, co_manager_ioctl_create_t *params, 
				 co_monitor_t **cmon_out);
extern co_rc_t co_monitor_destroy(co_monitor_t *cmon);

extern co_rc_t co_monitor_ioctl(co_monitor_t *cmon, co_manager_ioctl_monitor_t *io_buffer,
				unsigned long in_size, unsigned long out_size, 
				unsigned long *return_size, void **private_data);

extern co_rc_t co_monitor_alloc_pages(co_monitor_t *cmon, unsigned long pages, void **address);
extern co_rc_t co_monitor_free_pages(co_monitor_t *cmon, unsigned long pages, void *address);

extern co_rc_t co_monitor_malloc(co_monitor_t *cmon, unsigned long bytes, void **ptr);
extern co_rc_t co_monitor_free(co_monitor_t *cmon, void *ptr);

/*
 * An accessors to values of our core's kernel symbols.
 */
#define CO_MONITOR_KERNEL_SYMBOL(name)		        \
	((void *)(((char *)cmon->core_image) +	        \
		  (cmon->import.kernel_##name -	\
		   cmon->core_vaddr)))

#endif
