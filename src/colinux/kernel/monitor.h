/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
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
#include <colinux/os/kernel/mutex.h>
#include <colinux/os/kernel/timer.h>

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
	CO_MONITOR_STATE_START,
	CO_MONITOR_STATE_INITIALIZED,
	CO_MONITOR_STATE_RUNNING,
	CO_MONITOR_STATE_AWAITING_TERMINATION,
	CO_MONITOR_STATE_UNLOADING,
	CO_MONITOR_STATE_DOWN,
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
	 * Reference counting 
	 */
	unsigned long refcount;
	long blocking_threads;

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
	vm_ptr_t passage_page_vaddr;                  /* The virtual address of the 
							 PP in Linux */

	/*
	 * Core stuff (Linux kernel image) 
	 */
	void *core_image;                /* Host VA where we loaded our vmlinux */ 
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

	linux_pte_t *page_tables;            /* Linux's page tables that map the PPRAM.
					        A total of cmon_physical_frames PTEs. */
	unsigned long page_tables_size;      /* The number of bytes the page tables take */
	unsigned long page_tables_pages;     /* The number of pages the page tables take */

	/* 
	 * Dynamic allocations in the host
	 */

	unsigned long blocks_allocated;

	/* 
	 * Dynamic allocations in the PPRAM 
	 */

	unsigned long allocated_pages_num;   /* Number of pages we allocated from the host */
	unsigned long *pages_allocated;      /* Map of PPRAM PFN to how much we allocated 
						there */

	/*
	 * Reversed mapping between physical memory and virtual adderss.
	 */

	void **pa_to_host_va;        /* A map between a real physical PFN and an 
				        address of mapping in the host's kernel. 
				        co_monitor_host_memory_amount entries. */
	vm_ptr_t *pa_to_colx_va;     /* A map between a real physical PFN and an 
				        address of mapping in Linux's pseudo 
				        physical RAM */
	unsigned long pa_maps_size;  /* Size of these maps */
	unsigned long pa_maps_pages; /* Size of these maps in pages */

	/* 
	 * The bootmem
	 */

	unsigned long bootmem_pages; /* Number of empty bootmem pages to map after the core */
	void *bootmem;               /* Allocated bootmem (a total of bootmem_pages pages) */

	/*
	 * Page global directory
	 */
	linux_pgd_t pgd;             /* Pointer to the physical address PGD of 
					Linux (to be put in CR3 eventually */
	linux_pmd_t *pgd_page;	     /* Pointer to that PGD inside 'core_image' */

	/*
	 * Devices
	 */
	co_monitor_device_t devices[CO_DEVICES_TOTAL];

	/*
	 * Timer
	 */
	co_os_timer_t timer;

	/* 
	 * Block devices
	 */
	struct co_block_dev *block_devs[CO_MAX_BLOCK_DEVICES];

	/*
	 * Our virtaal VGA console: 
	 */
	co_os_mutex_t console_mutex;
	co_queue_t console_queue;
	co_console_t *console;
	unsigned long console_poll_cancel;
} co_monitor_t;

extern co_rc_t co_monitor_init(co_monitor_t *cmon, co_manager_ioctl_create_t *params);
extern co_rc_t co_monitor_load_section(co_monitor_t *cmon, co_monitor_ioctl_load_section_t *params);
extern co_rc_t co_monitor_run(co_monitor_t *cmon);
extern co_rc_t co_monitor_terminate(co_monitor_t *cmon);

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
