/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <asm/page.h>
#include <asm/pgtable.h>

#include <memory.h>

/*
 * Monitor
 *
 * FIXME: It should be architecture independant, but I am not sure if it
 * currently achieves that goal. Maybe with a few fixes...
 */

#include <colinux/common/debug.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/time.h>
#include <colinux/arch/passage.h>
#include <colinux/arch/interrupt.h>

#include "monitor.h"
#include "manager.h"
#include "block.h"
#include "fileblock.h"
#include "transfer.h"

co_rc_t co_monitor_malloc(co_monitor_t *cmon, unsigned long bytes, void **ptr)
{
	void *block = co_os_malloc(bytes);

	if (block == NULL)
		return CO_RC(OUT_OF_MEMORY);

	*ptr = block;

	cmon->blocks_allocated++;

	return CO_RC(OK);
}

co_rc_t co_monitor_free(co_monitor_t *cmon, void *ptr)
{
	co_os_free(ptr);

	cmon->blocks_allocated--;

	return CO_RC(OK);
}

static co_rc_t colinux_get_pfn(co_monitor_t *cmon, vm_ptr_t address, co_pfn_t *pfn)
{
	unsigned long current_pfn, pfn_group, pfn_index;

	current_pfn = (address >> PAGE_SHIFT);
	pfn_group = current_pfn / PTRS_PER_PTE;
	pfn_index = current_pfn % PTRS_PER_PTE;

	if (cmon->pp_pfns[pfn_group] == NULL)
		return CO_RC(ERROR);

	*pfn = cmon->pp_pfns[pfn_group][pfn_index];

	return CO_RC(OK);;
}

static void colinux_dump_page(unsigned long *page)
{
	int i = 0;

	while (i < PTRS_PER_PTE) {
		co_debug("%08x:  %08x %08x %08x %08x\n", 4*i, page[i], page[i+1], page[i+2], page[i+3]);
		i += 4;
	}
}

static co_rc_t colinux_dump_page_at_pfn(co_monitor_t *cmon, co_pfn_t pfn)
{
	unsigned long *page;

	page = (unsigned long *)co_os_map(cmon->manager, pfn);
	
	co_debug("dump of PFN %d\n", pfn);
	colinux_dump_page(page);

	co_os_unmap(cmon->manager, page);

	return CO_RC(OK);
}

static co_rc_t colinux_dump_page_at_address(co_monitor_t *cmon, vm_ptr_t address)
{
	co_rc_t rc;
	co_pfn_t pfn = 0;

	co_debug("dump of address %08x\n", address);
	rc = colinux_get_pfn(cmon, address, &pfn);
	if (!CO_OK(rc))
		return rc;

	return colinux_dump_page_at_pfn(cmon, pfn);
}

static co_rc_t colinux_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC(OK);
	co_pfn_t *pfns = NULL, self_map_pfn, passage_page_pfn, swapper_pg_dir_pfn;
	unsigned long pp_pagetables_pgd, self_map_page_offset, passage_page_offset;
	unsigned long reversed_physical_mapping_offset;
	
	rc = colinux_get_pfn(cmon, cmon->import.kernel_swapper_pg_dir, &swapper_pg_dir_pfn);
	if (!CO_OK(rc)) {
		co_debug("monitor: error getting swapper_pg_dir pfn (%x)\n", rc);
		goto out_error;
	}	
	
	cmon->pgd = __pgd(swapper_pg_dir_pfn << PAGE_SHIFT);
	co_debug_ulong(cmon->pgd);

	rc = co_monitor_arch_passage_page_alloc(cmon);
	if (!CO_OK(rc)) {
		co_debug("monitor: error allocating passage page (%d)\n", rc);
		goto out_error;
	}

	rc = co_monitor_arch_passage_page_init(cmon);
	if (!CO_OK(rc)) {
		co_debug("monitor: error initializing the passage page (%d)\n", rc);
		goto out_error;
	}

	pp_pagetables_pgd = CO_VPTR_PSEUDO_RAM_PAGE_TABLES >> PGDIR_SHIFT;
	pfns = cmon->pp_pfns[pp_pagetables_pgd];
	if (pfns == NULL) {
		co_debug("monitor: CO_VPTR_PSEUDO_RAM_PAGE_TABLES is not mapped, huh? (%x)\n");
		goto out_error;
	}

	rc = co_monitor_create_ptes(cmon, cmon->import.kernel_swapper_pg_dir, PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug("monitor: error initializing swapper_pg_dir (%x)\n", rc);
		goto out_error;
	}

	reversed_physical_mapping_offset = (CO_VPTR_PHYSICAL_TO_PSEUDO_PFN_MAP >> PGDIR_SHIFT)*sizeof(linux_pgd_t);
	co_debug_ulong(reversed_physical_mapping_offset);

	rc = co_monitor_copy_and_create_pfns(cmon, 
					     cmon->import.kernel_swapper_pg_dir + reversed_physical_mapping_offset, 
					     sizeof(linux_pgd_t)*cmon->manager->reversed_map_pgds_count, 
					     (void *)cmon->manager->reversed_map_pgds);
	if (!CO_OK(rc)) {
		co_debug("monitor: error adding reversed physical mapping (%x)\n", rc);
		goto out_error;
	}	

	rc = co_monitor_create_ptes(cmon, CO_VPTR_SELF_MAP, PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug("monitor: error initializing self map (%x)\n", rc);
		goto out_error;
	}

	rc = colinux_get_pfn(cmon, CO_VPTR_SELF_MAP, &self_map_pfn);
	if (!CO_OK(rc)) {
		co_debug("monitor: error getting self_map pfn (%x)\n", rc);
		goto out_error;
	}	

	self_map_page_offset = ((CO_VPTR_SELF_MAP >> PGDIR_SHIFT) * sizeof(linux_pte_t));
	co_debug_ulong(self_map_page_offset);

	rc = co_monitor_create_ptes(
		cmon, 
		cmon->import.kernel_swapper_pg_dir + self_map_page_offset, 
		sizeof(linux_pte_t), 
		&self_map_pfn);

	if (!CO_OK(rc)) {
		co_debug("monitor: error getting self_map pfn (%x)\n", rc);
		goto out_error;
	}	

	passage_page_offset = ((CO_VPTR_PASSAGE_PAGE & ((1 << PGDIR_SHIFT) - 1)) >> PAGE_SHIFT) * sizeof(linux_pte_t);
	co_debug_ulong(passage_page_offset);

	passage_page_pfn = co_os_virt_to_phys(cmon->passage_page) >> PAGE_SHIFT;
	co_debug_ulong(passage_page_pfn);

	rc = co_monitor_create_ptes(
		cmon, 
		CO_VPTR_SELF_MAP + passage_page_offset, 
		sizeof(linux_pte_t), 
		&passage_page_pfn);
	
	if (!CO_OK(rc)) {
		co_debug("monitor: error initializing self_map (%x)\n", rc);
		goto out_error;
	}	

	co_debug("monitor: initialization finished\n");

out_error:
	return rc;
}

static void colinux_free(co_monitor_t *cmon)
{
}

bool_t co_monitor_device_request(co_monitor_t *cmon, co_device_t device, unsigned long *params)
{
	switch (device) {
	case CO_DEVICE_BLOCK: {
		co_block_request_t *request;
		unsigned long dev_index = params[0];
		co_rc_t rc;

		request = (co_block_request_t *)(&params[1]);

		rc = co_monitor_block_request(cmon, dev_index, request);

		if (CO_OK(rc))
			request->rc = 0;
		else
			request->rc = -1;
		
		return PTRUE;
	}
	case CO_DEVICE_NETWORK: {
		co_network_request_t *network = NULL;

		network = (co_network_request_t *)(params);
		if (network->unit < 0  ||  network->unit >= CO_MODULE_MAX_CONET)
			break;

		memset(network->mac_address, 0, sizeof(network->mac_address));

		switch (network->type) {
		case CO_NETWORK_GET_MAC: {
			co_netdev_desc_t *dev = &cmon->config.net_devs[network->unit];
			if (dev->enabled == PFALSE)
				break;

			memcpy(network->mac_address, dev->mac_address, sizeof(network->mac_address));
			break;
		}
		default:
			break;
		}

		return PTRUE;
	}
	default:
		break;
	}	

	return PTRUE;
}

co_rc_t co_monitor_send_messages_to_linux(co_monitor_t *cmon)
{
	co_message_queue_item_t *message_item;
	co_message_t *message;
	unsigned long total_size;
	long num_messages;
	co_rc_t rc;

	co_passage_page->operation = CO_OPERATION_EMPTY;
	co_passage_page->params[0] = 0;

	num_messages = co_queue_size(&cmon->linux_message_queue);
	if (num_messages == 0)
		return CO_RC(OK);

	rc = co_queue_pop_tail(&cmon->linux_message_queue, (void **)&message_item);
	if (!CO_OK(rc))
		return rc;

	message = message_item->message;
	total_size = message->size + sizeof(*message);

	if (total_size > 2000)
		return CO_RC(OK);

	co_passage_page->operation = CO_OPERATION_MESSAGE_FROM_MONITOR;
	co_passage_page->params[0] = num_messages - 1;
	memcpy(&co_passage_page->params[1], message, total_size);

	co_queue_free(&cmon->linux_message_queue, message_item);
	co_os_free(message);

	return CO_RC(OK);
}

void co_monitor_debug_line(co_monitor_t *cmon, const char *text)
{
	struct {
		co_message_t message;
		co_daemon_message_t payload;
		char data[0];
	} *message = NULL;
	unsigned long size;

	size = sizeof(message->payload) + strlen(text) + 1;
	message = (typeof(message))co_os_malloc(size + sizeof(message->message));
	if (message == NULL)
		return;

	message->message.from = CO_MODULE_MONITOR;
	message->message.to = CO_MODULE_DAEMON;
	message->message.priority = CO_PRIORITY_IMPORTANT;
	message->message.type = CO_MESSAGE_TYPE_STRING;
	message->message.size = size;
	message->payload.type = CO_MONITOR_MESSAGE_TYPE_DEBUG_LINE;
	memcpy(message->data, text, strlen(text)+1);

	co_message_switch_message(&cmon->message_switch, &message->message);
}

void co_unsigned_long_to_hex(char *text, unsigned long number)
{
	int count = 8;

	text += count;
	while (count) {
		int digit = number & 0xf;

		count--;
		*text = (digit >= 10) ? (digit + 'a') : (digit + '0');
		number >>= 4;
		text--;
	}
}

bool_t co_monitor_iteration(co_monitor_t *cmon)
{
	switch (co_passage_page->operation) {
	case CO_OPERATION_FORWARD_INTERRUPT: 
	case CO_OPERATION_IDLE: 
		co_monitor_send_messages_to_linux(cmon);
		break;
	}

	co_switch();

	switch (co_passage_page->operation) {
	case CO_OPERATION_FORWARD_INTERRUPT: {
		co_monitor_arch_real_hardware_interrupt(cmon);
		return PFALSE;
	}
	case CO_OPERATION_TERMINATE: {
		struct {
			co_message_t message;
			co_daemon_message_t payload;
		} message;
		
		co_debug("monitor: linux terminated (%d)\n", co_passage_page->params[0]);
		message.message.from = CO_MODULE_MONITOR;
		message.message.to = CO_MODULE_DAEMON;
		message.message.priority = CO_PRIORITY_IMPORTANT;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message.payload);
		message.payload.type = CO_MONITOR_MESSAGE_TYPE_TERMINATED;
		message.payload.terminated.reason = co_passage_page->params[0];

		co_message_switch_dup_message(&cmon->message_switch, &message.message);

		cmon->state = CO_MONITOR_STATE_TERMINATED;
		return PFALSE;
	}
	case CO_OPERATION_MESSAGE_FROM_MONITOR: {
		co_monitor_send_messages_to_linux(cmon);
		return PTRUE;
	}
	case CO_OPERATION_IDLE: {
		co_message_t message;
		
		message.from = CO_MODULE_MONITOR;
		message.to = CO_MODULE_IDLE;
		message.priority = CO_PRIORITY_DISCARDABLE;
		message.type = CO_MESSAGE_TYPE_STRING;
		message.size = 0;

		co_message_switch_dup_message(&cmon->message_switch, &message);

		return PFALSE;
	}

        case CO_OPERATION_DEBUG_LINE: {
		char *p = (char *)&co_passage_page->params[0];
		p[0x200] = '\0';
		co_monitor_debug_line(cmon, p);

#if (1)	
		{
			/* DEBUG HACK */
			char buf[0x50];
			memset(buf, ' ', sizeof(buf));

			co_unsigned_long_to_hex(&buf[0], co_passage_page->linuxvm_state.cs);
			co_unsigned_long_to_hex(&buf[0x9*1], co_passage_page->linuxvm_state.ds);
			co_unsigned_long_to_hex(&buf[0x9*2], co_passage_page->linuxvm_state.es);
			co_unsigned_long_to_hex(&buf[0x9*3], co_passage_page->linuxvm_state.fs);
			co_unsigned_long_to_hex(&buf[0x9*4], co_passage_page->linuxvm_state.gs);
			co_unsigned_long_to_hex(&buf[0x9*5], co_passage_page->linuxvm_state.ss);
			co_unsigned_long_to_hex(&buf[0x9*6], co_passage_page->linuxvm_state.ldt);
			buf[0x9*7] = '\n';
			buf[0x9*7+1] = '\0';

			co_monitor_debug_line(cmon, buf);
		}
#endif

                return PFALSE;
        }

	case CO_OPERATION_MESSAGE_TO_MONITOR: {
		co_message_t *message;
		co_rc_t rc;

		message = (co_message_t *)(&co_passage_page->params[1]);

#if (0)
		if (message->to != 3)
			co_debug("monitor: message: %d:%d [%d] <%d> (%d) '%c%c%c'\n", 
				 message->from, message->to, message->priority,
				 message->type, message->size, 
				 message->size >= 1 ? message->data[0] : ' ',
				 message->size >= 2 ? message->data[1] : ' ',
				 message->size >= 3 ? message->data[2] : ' ');
#endif

		rc = co_message_switch_dup_message(&cmon->message_switch, message);

		return PFALSE;
	}
	case CO_OPERATION_DEVICE: {
		unsigned long device = co_passage_page->params[0];

		return co_monitor_device_request(cmon, device, &co_passage_page->params[1]);
	}
	case CO_OPERATION_GET_TIME: {
		co_passage_page->params[0] = co_os_get_time();

		return PTRUE;
	}
	case CO_OPERATION_GET_HIGH_PREC_QUOTIENT: {
		co_passage_page->params[0] = co_os_get_high_prec_quotient();

		return PTRUE;
	}
	default:
		co_debug("operation %d not handled\n", co_passage_page->operation);
		return PFALSE;
	}
	
	return PTRUE;
}

void co_monitor_free_file_blockdevice(co_monitor_t *cmon, co_block_dev_t *dev)
{
	co_monitor_file_block_dev_t *fdev = (co_monitor_file_block_dev_t *)dev;

	co_monitor_file_block_shutdown(fdev);
	co_monitor_free(cmon, dev);
}

co_rc_t co_monitor_load_configuration(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK; 
	long i;

	for (i=0; i < CO_MAX_BLOCK_DEVICES; i++) {
		co_monitor_file_block_dev_t *dev;
		co_block_dev_desc_t *conf_dev = &cmon->config.block_devs[i];
		if (!conf_dev->enabled)
			continue;

		rc = co_monitor_malloc(cmon, sizeof(co_monitor_file_block_dev_t), (void **)&dev);
		if (!CO_OK(rc))
			goto out;

		rc = co_monitor_file_block_init(dev, &conf_dev->pathname);
		if (CO_OK(rc)) {
			co_monitor_block_register_device(cmon, i, (co_block_dev_t *)dev);
			dev->dev.free = co_monitor_free_file_blockdevice;
		} else {
			co_monitor_free(cmon, dev);
			goto out;
		}
	}

out:
	if (!CO_OK(rc))
		co_monitor_unregister_and_free_block_devices(cmon);

	return rc;
}

static void timer_callback(void *data)
{
        co_monitor_t *cmon = (co_monitor_t *)data;

        cmon->timer_interrupt = PTRUE;
}

static void free_pseudo_physical_memory(co_monitor_t *monitor)
{
	int i, j;

	co_debug("monitor: freeing page frames for pseudo physical RAM\n");

	for (i=0; i < PTRS_PER_PGD; i++) {
		if (!monitor->pp_pfns[i])
			continue;

		for (j=0; j < PTRS_PER_PTE; j++)
			if (monitor->pp_pfns[i][j] != 0)
				co_os_put_page(monitor->manager, monitor->pp_pfns[i][j]);

		co_monitor_free(monitor, monitor->pp_pfns[i]);
	}

	co_monitor_free(monitor, monitor->pp_pfns);
	monitor->pp_pfns = NULL;

	co_debug("monitor: done freeing\n");
}

static co_rc_t alloc_pseudo_physical_memory(co_monitor_t *monitor)
{
	co_rc_t rc;
	int i, j;
	co_pfn_t pseudo_pfn = 0, first_pp_pgd;

	co_debug("monitor: allocating page frames for pseudo physical RAM...\n");

	rc = co_monitor_malloc(monitor, sizeof(co_pfn_t *)*PTRS_PER_PGD, (void **)&monitor->pp_pfns);
	if (!CO_OK(rc))
		return rc;

	memset(monitor->pp_pfns, 0, sizeof(co_pfn_t *)*PTRS_PER_PGD);

	rc = co_monitor_scan_and_create_pfns(monitor, __PAGE_OFFSET, monitor->physical_frames << PAGE_SHIFT);
	if (!CO_OK(rc)) {
		free_pseudo_physical_memory(monitor);
		return rc;
	}

	co_debug("monitor: setting reversed map\n");

	for (i=0; i < PTRS_PER_PGD; i++) {
		if (monitor->pp_pfns[i] == NULL)
			continue;

		for (j=0; j < PTRS_PER_PTE; j++) {
			co_pfn_t real_pfn = monitor->pp_pfns[i][j];

			if (real_pfn != 0) {
				rc = co_manager_set_reversed_pfn(monitor->manager, real_pfn, pseudo_pfn);
				if (!CO_OK(rc)) {
					free_pseudo_physical_memory(monitor);
					return rc;
				}
			}

			pseudo_pfn++;
		}
	}

	co_debug("monitor: creating page tables\n");

	first_pp_pgd = __PAGE_OFFSET >> PGDIR_SHIFT;
	for (i=first_pp_pgd; i < PTRS_PER_PGD; i++) {
		co_pfn_t *pfns = monitor->pp_pfns[i];
		unsigned long address;

		if (!pfns)
			break;

		address = CO_VPTR_PSEUDO_RAM_PAGE_TABLES + ((i - first_pp_pgd) * PAGE_SIZE);

		co_debug("monitor: creating one page table (at %x)\n", address);

		rc = co_monitor_create_ptes(monitor, address, PAGE_SIZE, pfns);
		if (!CO_OK(rc)) {
			free_pseudo_physical_memory(monitor);
			return rc;
		}
	}

	return rc;
}

co_rc_t co_monitor_create(co_manager_t *manager, co_manager_ioctl_create_t *params, co_monitor_t **cmon_out)
{
	co_symbols_import_t *import = &params->import;
	co_monitor_t *cmon;
	co_rc_t rc = CO_RC_OK;

	manager->monitors_count++;

	cmon = co_os_malloc(sizeof(*cmon));
	if (!cmon) {
		rc = CO_RC(OUT_OF_MEMORY);
		goto out;
	}

	memset(cmon, 0, sizeof(*cmon));
	cmon->manager = manager;
	cmon->state = CO_MONITOR_STATE_INITIALIZED;
	cmon->console_state = CO_MONITOR_CONSOLE_STATE_DETACHED;

	co_message_switch_init(&cmon->message_switch, CO_MODULE_KERNEL_SWITCH);

	rc = co_queue_init(&cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_PRINTK, &cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_DAEMON, &cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_IDLE, &cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_USER_SWITCH, &cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_CONSOLE, &cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_queue_init(&cmon->linux_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_message_switch_set_rule_queue(&cmon->message_switch, CO_MODULE_LINUX, &cmon->linux_message_queue);
	if (!CO_OK(rc))
		goto out_free_user_message_queue;

	rc = co_monitor_os_init(cmon);
	if (!CO_OK(rc))
		goto out_free_linux_message_queue;

	cmon->core_vaddr = import->kernel_start;
	cmon->core_pages = (import->kernel_end - import->kernel_start + PAGE_SIZE - 1) >> PAGE_SHIFT;
	cmon->core_end = cmon->core_vaddr + (cmon->core_pages << PAGE_SHIFT);
	cmon->import = params->import;
	cmon->config = params->config;

	if (cmon->config.ram_size == 0) {
		/* Use default RAM sizes */
	
		/*
		 * FIXME: Be careful with that. We don't want to exhaust the host OS's memory
		 * pools because it can destablized it. 
		 *
		 * We need to find the minimum requirement for size of the coLinux RAM 
		 * for achieving the best performance assuming that the host OS is 
		 * caching away the coLinux swap device into the host system's other
		 * RAM.
		 */

		if (cmon->manager->host_memory_amount >= 128*1024*1024) {
			/* Use quarter */
			cmon->memory_size = cmon->manager->host_memory_amount/(1024*1024*4);
		} else {
			cmon->memory_size = 16;
		}
	} else {
		cmon->memory_size = cmon->config.ram_size;
	}

	co_debug("monitor: configured to %d MB\n", cmon->memory_size);

	if (cmon->memory_size < 8)
		cmon->memory_size = 8;
	else if (cmon->memory_size > 512)
		cmon->memory_size = 512;

	co_debug("monitor: after adjustments: %d MB\n", cmon->memory_size);

	/*
	 * FIXME: Currently the number of bootmem pages is hardcored, but we can easily
	 * modify this to depend on the amount of pseudo physical RAM, which
	 * is approximately sizeof(struct page)*(pseudo physical pages) plus
	 * more smaller structures. 
	 */
	cmon->memory_size <<= 20; /* Megify */

	cmon->physical_frames = cmon->memory_size >> PAGE_SHIFT;
	cmon->end_physical = __PAGE_OFFSET + cmon->memory_size;
	cmon->passage_page_vaddr = CO_VPTR_PASSAGE_PAGE;

	rc = alloc_pseudo_physical_memory(cmon);
        if (!CO_OK(rc)) {
		goto out_free_os_dep;
	}

        rc = co_os_timer_create(&timer_callback, cmon, 10, &cmon->timer);
        if (!CO_OK(rc)) {
                co_debug("monitor: creating host OS timer (%d)\n", rc);
                goto out_free_pp;
        }

	rc = co_monitor_load_configuration(cmon);
	if (!CO_OK(rc)) {
		co_debug("monitor: loading monitor configuration (%d)\n", rc);
		goto out_destroy_timer;
	}

	cmon->state = CO_MONITOR_STATE_INITIALIZED;

	*cmon_out = cmon;

	return CO_RC(OK);

/* error path */
out_destroy_timer:
        co_os_timer_destroy(cmon->timer);

out_free_pp:
	free_pseudo_physical_memory(cmon);

out_free_os_dep:
	co_monitor_os_exit(cmon);

out_free_linux_message_queue:
	co_queue_flush(&cmon->linux_message_queue);

out_free_user_message_queue:
	co_queue_flush(&cmon->user_message_queue);

out_free:
	co_message_switch_free(&cmon->message_switch);
	co_os_free(cmon);

out:
	manager->monitors_count--;
	return rc;
}

co_rc_t co_monitor_load_section(co_monitor_t *cmon, co_monitor_ioctl_load_section_t *params)
{
	co_rc_t rc = CO_RC(OK);

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED)
		return CO_RC(ERROR);

	if (params->user_ptr) {
		co_debug("monitor: loading section at 0x%x (0x%x bytes)\n", params->address, params->size);
		rc = co_monitor_copy_and_create_pfns(
			cmon, params->address, params->size, params->buf);
	}

	return rc;
}

co_rc_t co_monitor_destroy(co_monitor_t *cmon)
{
	co_manager_t *manager;
	
	co_debug("monitor: cleaning up\n");
	co_debug("monitor: before free: %d blocks\n", cmon->blocks_allocated);

	manager = cmon->manager;

	if (cmon->state >= CO_MONITOR_STATE_RUNNING) {
                co_os_timer_deactivate(cmon->timer);
		colinux_free(cmon);
	}

	co_monitor_unregister_and_free_block_devices(cmon);
	free_pseudo_physical_memory(cmon);
	co_monitor_os_exit(cmon);
	co_queue_flush(&cmon->linux_message_queue);
	co_queue_flush(&cmon->user_message_queue);
	co_message_switch_free(&cmon->message_switch);

	co_debug("monitor: after free: %d blocks\n", cmon->blocks_allocated);

	co_os_free(cmon);

	manager->monitors_count--;

	return CO_RC_OK;
}

co_rc_t co_monitor_start(co_monitor_t *cmon)
{
	co_rc_t rc;

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED) {
		co_debug("monitor: invalid state\n");
		return CO_RC(ERROR);
	}
		
	rc = colinux_init(cmon);
	if (!CO_OK(rc)) {
		co_debug("monitor: error initializing coLinux context (%d)\n", rc);
		return rc;
	}

	co_os_timer_activate(cmon->timer);

	co_passage_page->operation = CO_OPERATION_START;
	co_passage_page->params[0] = cmon->core_end;
	co_passage_page->params[1] = 0x800;
	co_passage_page->params[2] = cmon->memory_size;
	co_passage_page->params[3] = 0; /* cmon->manager->pa_maps_size; */

	memcpy(&co_passage_page->params[4], cmon->config.boot_parameters_line, 
	       sizeof(cmon->config.boot_parameters_line));

	cmon->state = CO_MONITOR_STATE_RUNNING;

	return CO_RC(OK);
}


co_rc_t co_monitor_run(co_monitor_t *cmon,
		       co_monitor_ioctl_run_t *params,
		       unsigned long out_size,
		       unsigned long *return_size)
{
	char *params_data;
	int i;

	/* Get messages from userspace */
	params_data = params->data;

	for (i=0; i < params->num_messages; i++) {
		co_message_t *message = (typeof(message))params_data;

		co_message_switch_dup_message(&cmon->message_switch, message);
		
		params_data += message->size + sizeof(*message);
	}

	if (cmon->state == CO_MONITOR_STATE_RUNNING) {
		/* Switch back and forth from Linux */

		while (co_monitor_iteration(cmon));
	}

	/* Flush messages to userspace */
	*return_size = sizeof(*params);
	params->num_messages = 0;

	if (co_queue_size(&cmon->user_message_queue) == 0)
		return CO_RC(OK);

	*return_size = 0;

	co_message_write_queue(&cmon->user_message_queue, params->data, 
			       out_size - sizeof(*params), &params->num_messages,
			       return_size);
		
	*return_size += sizeof(*params);

	return CO_RC(OK);
}

co_rc_t co_monitor_ioctl(co_monitor_t *cmon, co_manager_ioctl_monitor_t *io_buffer,
			 unsigned long in_size, unsigned long out_size, 
			 unsigned long *return_size, void **private_data)
{
	co_rc_t rc = CO_RC_ERROR;

	switch (io_buffer->op) {
	case CO_MONITOR_IOCTL_DESTROY: {
		co_monitor_destroy(cmon);
		*private_data = NULL;
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_LOAD_SECTION: {
		return co_monitor_load_section(cmon, (co_monitor_ioctl_load_section_t *)io_buffer);
	}
	case CO_MONITOR_IOCTL_START: {
		return co_monitor_start(cmon);
	}
	case CO_MONITOR_IOCTL_RUN: {
		return co_monitor_run(cmon, (co_monitor_ioctl_run_t *)io_buffer, out_size, return_size);
	}
	default:
		return rc;
	}

	return rc;
}
