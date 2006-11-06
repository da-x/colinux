/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/*
 * Monitor
 *
 * FIXME: It should be architecture independant, but I am not sure if it
 * currently achieves that goal. Maybe with a few fixes...
 */

#include <colinux/common/debug.h>
#include <colinux/common/libc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>
#include <colinux/os/kernel/monitor.h>
#include <colinux/os/kernel/time.h>
#include <colinux/os/kernel/wait.h>
#include <colinux/os/timer.h>
#include <colinux/os/current/kernel/main.h>
#include <colinux/arch/passage.h>
#include <colinux/arch/interrupt.h>
#include <colinux/arch/mmu.h>

#include "monitor.h"
#include "manager.h"
#include "block.h"
#include "fileblock.h"
#include "transfer.h"
#include "filesystem.h"
#include "pages.h"

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

static co_rc_t guest_address_space_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC(OK);
	co_pfn_t *pfns = NULL, self_map_pfn, passage_page_pfn, swapper_pg_dir_pfn;
	unsigned long pp_pagetables_pgd, self_map_page_offset, passage_page_offset;
	unsigned long reversed_physical_mapping_offset;

	rc = co_monitor_get_pfn(cmon, cmon->import.kernel_swapper_pg_dir, &swapper_pg_dir_pfn);
	if (!CO_OK(rc)) {
		co_debug_error("error getting swapper_pg_dir pfn (%x)\n", rc);
		goto out_error;
	}	
	
	cmon->pgd = swapper_pg_dir_pfn << CO_ARCH_PAGE_SHIFT;

	rc = co_monitor_arch_passage_page_alloc(cmon);
	if (!CO_OK(rc)) {
		co_debug_error("error allocating passage page (%d)\n", rc);
		goto out_error;
	}

	rc = co_monitor_arch_passage_page_init(cmon);
	if (!CO_OK(rc)) {
		co_debug_error("error initializing the passage page (%d)\n", rc);
		goto out_error;
	}

	pp_pagetables_pgd = CO_VPTR_PSEUDO_RAM_PAGE_TABLES >> PGDIR_SHIFT;
	pfns = cmon->pp_pfns[pp_pagetables_pgd];
	if (pfns == NULL) {
		co_debug_error("CO_VPTR_PSEUDO_RAM_PAGE_TABLES is not mapped, huh? (%x)\n");
		goto out_error;
	}

	rc = co_monitor_create_ptes(cmon, cmon->import.kernel_swapper_pg_dir, CO_ARCH_PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug_error("error initializing swapper_pg_dir (%x)\n", rc);
		goto out_error;
	}

	reversed_physical_mapping_offset = (CO_VPTR_PHYSICAL_TO_PSEUDO_PFN_MAP >> PGDIR_SHIFT)*sizeof(linux_pgd_t);

	rc = co_monitor_copy_and_create_pfns(cmon, 
					     cmon->import.kernel_swapper_pg_dir + reversed_physical_mapping_offset, 
					     sizeof(linux_pgd_t)*cmon->manager->reversed_map_pgds_count, 
					     (void *)cmon->manager->reversed_map_pgds);
	if (!CO_OK(rc)) {
		co_debug_error("error adding reversed physical mapping (%x)\n", rc);
		goto out_error;
	}	

	rc = co_monitor_create_ptes(cmon, CO_VPTR_SELF_MAP, CO_ARCH_PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug_error("error initializing self_map (%x)\n", rc);
		goto out_error;
	}

	rc = co_monitor_get_pfn(cmon, CO_VPTR_SELF_MAP, &self_map_pfn);
	if (!CO_OK(rc)) {
		co_debug_error("error getting self_map pfn (%x)\n", rc);
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
		co_debug_error("error getting self_map pfn (%x)\n", rc);
		goto out_error;
	}	

	passage_page_offset = ((CO_VPTR_PASSAGE_PAGE & ((1 << PGDIR_SHIFT) - 1)) >> CO_ARCH_PAGE_SHIFT) * sizeof(linux_pte_t);
	passage_page_pfn = co_os_virt_to_phys(cmon->passage_page) >> CO_ARCH_PAGE_SHIFT;

	rc = co_monitor_create_ptes(
		cmon, 
		CO_VPTR_SELF_MAP + passage_page_offset, 
		sizeof(linux_pte_t), 
		&passage_page_pfn);

	if (!CO_OK(rc)) {
		co_debug_error("error mapping passage page into the self map (%x)\n", rc);
		goto out_error;
	}	

	rc = co_monitor_create_ptes(
		cmon, 
		CO_VPTR_SELF_MAP, 
		sizeof(linux_pte_t), 
		&self_map_pfn);
	
	if (!CO_OK(rc)) {
		co_debug_error("error initializing self_map (%x)\n", rc);
		goto out_error;
	}	

	long io_buffer_page = 0;
	long io_buffer_num_pages = CO_VPTR_IO_AREA_SIZE >> CO_ARCH_PAGE_SHIFT;
	long io_buffer_offset = ((CO_VPTR_IO_AREA_START & ((1 << PGDIR_SHIFT) - 1)) >> CO_ARCH_PAGE_SHIFT) 
		* sizeof(linux_pte_t);
	unsigned long io_buffer_host_address = (unsigned long)(cmon->io_buffer);

	for (io_buffer_page=0; io_buffer_page < io_buffer_num_pages; io_buffer_page++) {
		unsigned long io_buffer_pfn = co_os_virt_to_phys((void *)io_buffer_host_address) >> CO_ARCH_PAGE_SHIFT;
		
		rc = co_monitor_create_ptes(cmon, CO_VPTR_SELF_MAP + io_buffer_offset,
					    sizeof(linux_pte_t), &io_buffer_pfn);
		if (!CO_OK(rc)) {
			co_debug_error("error initializing io buffer (%x, %d)\n", rc, io_buffer_page);
			goto out_error;
		}
		
		io_buffer_offset += sizeof(linux_pte_t);
		io_buffer_host_address += CO_ARCH_PAGE_SIZE;
	}

	co_debug("initialization finished\n");

out_error:
	return rc;
}

static bool_t device_request(co_monitor_t *cmon, co_device_t device, unsigned long *params)
{
	switch (device) {
	case CO_DEVICE_BLOCK: {
		co_block_request_t *request;
		unsigned long dev_index = params[0];
		co_rc_t rc;

		co_debug_lvl(blockdev, 13, "blockdev requested (unit %d)\n", dev_index);

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

		co_debug_lvl(network, 13, "network requested\n");

		network = (co_network_request_t *)(params);
		network->result = 0;

		if (network->unit < 0  ||  network->unit >= CO_MODULE_MAX_CONET) {
			co_debug_lvl(network, 12, "invalid network unit %d\n", network->unit);
			break;
		}

		co_debug_lvl(network, 12, "network unit %d requested\n", network->unit);

		switch (network->type) {
		case CO_NETWORK_GET_MAC: {
			co_netdev_desc_t *dev = &cmon->config.net_devs[network->unit];

			co_debug_lvl(network, 10, "CO_NETWORK_GET_MAC requested\n");

			network->result = dev->enabled;

			if (dev->enabled == PFALSE)
				break;

			co_memcpy(network->mac_address, dev->mac_address, sizeof(network->mac_address));
			break;
		}
		default:
			break;
		}

		return PTRUE;
	}
	case CO_DEVICE_FILESYSTEM: {
		co_monitor_file_system(cmon, params[0], params[1], &params[2]);
		return PTRUE;
	}
	default:
		break;
	}	

	return PTRUE;
}

static co_rc_t callback_return_messages(co_monitor_t *cmon)
{	
	co_rc_t rc;
	unsigned char *io_buffer, *io_buffer_end;
	co_queue_t *queue;

	co_passage_page->params[0] = 0;

	if (!cmon->io_buffer)
		return CO_RC(ERROR);

	if (cmon->io_buffer->messages_waiting)
		return CO_RC(OK);

	io_buffer = cmon->io_buffer->buffer;
	io_buffer_end = io_buffer + CO_VPTR_IO_AREA_SIZE - sizeof(co_io_buffer_t);

	co_os_mutex_acquire(cmon->linux_message_queue_mutex);
	
	cmon->io_buffer->messages_waiting = 0;

	queue = &cmon->linux_message_queue;
	while (co_queue_size(queue) != 0)
	{
		co_message_queue_item_t *message_item;
		rc = co_queue_peek_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			return rc;
		
		co_message_t *message = message_item->message;
		unsigned long size = message->size + sizeof(*message);

		if (message->from == CO_MODULE_CONET0)
			co_debug_lvl(network, 14, "message sent to linux: %x", message);
		
		if (io_buffer + size > io_buffer_end) {
			break;
		}
		
		rc = co_queue_pop_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			return rc;

		if ((unsigned long)message->from >= (unsigned long)CO_MODULES_MAX) {
			co_debug_system("BUG! %s:%d\n", __FILE__, __LINE__);
			co_queue_free(queue, message_item);
			co_os_free(message);
			break;
		}

		if ((unsigned long)message->to >= (unsigned long)CO_MODULES_MAX){
			co_debug_system("BUG! %s:%d\n", __FILE__, __LINE__);
			co_queue_free(queue, message_item);
			co_os_free(message);
			break;
		}

		co_linux_message_t *linux_message = (co_linux_message_t *)message->data;
		if ((unsigned long)linux_message->device >= (unsigned long)CO_DEVICES_TOTAL){
			co_debug_system("BUG! %s:%d %d %d\n", __FILE__, __LINE__,
					message->to, message->from);
			co_queue_free(queue, message_item);
			co_os_free(message);
			break;
		}

		cmon->io_buffer->messages_waiting += 1;
		co_queue_free(queue, message_item);
		co_memcpy(io_buffer, message, size);
		io_buffer += size;
		co_os_free(message);
	}

	co_os_mutex_release(cmon->linux_message_queue_mutex);

	co_passage_page->params[0] = io_buffer - (unsigned char *)&cmon->io_buffer->buffer;

	co_debug_lvl(messages, 12, "sending messages to linux (%d bytes)\n", co_passage_page->params[0]);

	return CO_RC(OK);
}

static co_rc_t callback_return_jiffies(co_monitor_t *cmon)
{	
	co_timestamp_t timestamp;
	long long timestamp_diff;
	unsigned long jiffies = 0;

	co_os_get_timestamp(&timestamp);

	timestamp_diff = cmon->timestamp_reminder;
	timestamp_diff += 100 * (((long long)timestamp.quad) - ((long long)cmon->timestamp.quad));

	jiffies = co_div64(timestamp_diff, cmon->timestamp_freq.quad);
	cmon->timestamp_reminder = timestamp_diff - (jiffies * cmon->timestamp_freq.quad);
	cmon->timestamp = timestamp;

	co_passage_page->params[1] = jiffies;

	return CO_RC(OK);
}

static co_rc_t callback_return(co_monitor_t *cmon)
{
	co_passage_page->operation = CO_OPERATION_MESSAGE_FROM_MONITOR;

	callback_return_messages(cmon);
	callback_return_jiffies(cmon);

	return CO_RC(OK);
}

static bool_t co_terminate(co_monitor_t *cmon)
{
	co_os_timer_deactivate(cmon->timer);

	cmon->termination_reason = co_passage_page->params[0];

	if (cmon->termination_reason == CO_TERMINATE_BUG) {
		int len;
		char *str = (char *)&co_passage_page->params[4];
		cmon->bug_info.code = co_passage_page->params[1];
		cmon->bug_info.line = co_passage_page->params[2];

		len = co_strlen(str);
		if (len < sizeof(cmon->bug_info.file)-1)
			co_snprintf(cmon->bug_info.file, sizeof(cmon->bug_info.file), "%s", str);
		else
			/* show the end of filename */
			co_snprintf(cmon->bug_info.file, sizeof(cmon->bug_info.file),
				    "...%s", str + len - sizeof(cmon->bug_info.file)-1 - 3);

		co_debug_system("BUG%d at %s:%d\n", cmon->bug_info.code,
				cmon->bug_info.file, cmon->bug_info.line);
	}

	cmon->state = CO_MONITOR_STATE_TERMINATED;

	return PFALSE;
}

static bool_t co_idle(co_monitor_t *cmon)
{
	bool_t next_iter = PTRUE;
	co_queue_t *queue;

	co_os_wait_sleep(cmon->idle_wait);

	queue = &cmon->linux_message_queue;
	if (co_queue_size(queue) == 0)
		next_iter = PFALSE;

	return next_iter;
}

static void co_free_pages(co_monitor_t *cmon, vm_ptr_t address, int num_pages)
{
	unsigned long scan_address;
	int j;

	scan_address = address;
	for (j=0; j < num_pages; j++) {
		co_monitor_free_and_unmap_page(cmon, scan_address);
		scan_address += CO_ARCH_PAGE_SIZE;
	}
}

static co_rc_t co_alloc_pages(co_monitor_t *cmon, vm_ptr_t address, int num_pages)
{
	unsigned long scan_address;
	co_rc_t rc = CO_RC(OK);
	int i;

	scan_address = address;
	for (i=0; i < num_pages; i++) {
		rc = co_monitor_alloc_and_map_page(cmon, scan_address);
		if (!CO_OK(rc))
			break;
		
		scan_address += CO_ARCH_PAGE_SIZE;
	}

	if (!CO_OK(rc)) {
		num_pages = i;
		scan_address = address;
		for (i=0; i < num_pages; i++) {
			co_monitor_free_and_unmap_page(cmon, scan_address);
			scan_address += CO_ARCH_PAGE_SIZE;
		}
	}

	return rc;
}

static void incoming_message(co_monitor_t *cmon, co_message_t *message)
{
	co_manager_open_desc_t opened;
	co_rc_t rc;

	if (message->to == CO_MODULE_CONET0)
		co_debug_lvl(network, 14, "message received: %x %x", cmon, message);
	
	co_os_mutex_acquire(cmon->connected_modules_write_lock);
	opened = cmon->connected_modules[message->to];
	if (opened != NULL)
		rc = co_manager_open_ref(opened);
	else
		rc = CO_RC(ERROR);
	co_os_mutex_release(cmon->connected_modules_write_lock);
	
	if (CO_OK(rc)) {
		co_manager_send(cmon->manager, opened, message);
		co_manager_close(cmon->manager, opened);
	}

	if (message->to == CO_MODULE_CONET0)
		co_debug_lvl(network, 14, "message received end: %x %x", cmon, message);

	switch (message->to) {
	case CO_MODULE_CONSOLE:
		if (message->from == CO_MODULE_LINUX) {
			co_console_op(cmon->console, (co_console_message_t *)message->data);
		}
		break;
	default:
		break;
	}
}

co_rc_t co_monitor_message_from_user(co_monitor_t *monitor, co_manager_open_desc_t opened, co_message_t *message)
{
	co_rc_t rc = CO_RC(OK);

	if (message->to == CO_MODULE_LINUX) {
		co_os_mutex_acquire(monitor->linux_message_queue_mutex);
		rc = co_message_dup_to_queue(message, &monitor->linux_message_queue);
		co_os_mutex_release(monitor->linux_message_queue_mutex);
		co_os_wait_wakeup(monitor->idle_wait);
	} else {
		rc = CO_RC(ERROR);
	}

	return rc;
}


/*
 * iteration - returning PTRUE means that the driver will return 
 * immediately to Linux instead of returning to the host's 
 * userspace and only then to Linux.
 */

static bool_t iteration(co_monitor_t *cmon)
{
	switch (co_passage_page->operation) {
	case CO_OPERATION_FORWARD_INTERRUPT: 
	case CO_OPERATION_IDLE: 
		callback_return(cmon);
		break;
	}

	co_debug_lvl(context_switch, 14, "switching to linux (%d)\n", co_passage_page->operation);
	co_host_switch_wrapper(cmon);

	if (co_passage_page->operation == CO_OPERATION_FORWARD_INTERRUPT)
		co_monitor_arch_real_hardware_interrupt(cmon);
	else
		co_monitor_arch_enable_interrupts();

	switch (co_passage_page->operation) {
	case CO_OPERATION_FREE_PAGES: {
		co_free_pages(cmon, co_passage_page->params[0], co_passage_page->params[1]);
		return PTRUE;
	}

	case CO_OPERATION_ALLOC_PAGES: {
		co_rc_t rc;
		rc = co_alloc_pages(cmon, co_passage_page->params[0], co_passage_page->params[1]);
		co_passage_page->params[4] = (unsigned long)(rc);
		return PTRUE;
	}

	case CO_OPERATION_FORWARD_INTERRUPT: {
		co_debug_lvl(context_switch, 15, "switching from linux (CO_OPERATION_FORWARD_INTERRUPT), %d\n",
			     cmon->timer_interrupt);

		if (cmon->timer_interrupt) {
			cmon->timer_interrupt = PFALSE;
			/* Return to userspace once in a while */
			return PFALSE;
		}

		return PTRUE;
	}
	case CO_OPERATION_TERMINATE: 
		return co_terminate(cmon);

	case CO_OPERATION_IDLE:
		return co_idle(cmon);

	case CO_OPERATION_MESSAGE_TO_MONITOR: {
		co_message_t *message;
		
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_MESSAGE_TO_MONITOR)\n");

		message = (co_message_t *)cmon->io_buffer->buffer;
		if (message  &&  message->to < CO_MONITOR_MODULES_COUNT)
			incoming_message(cmon, message);

		cmon->io_buffer->messages_waiting = 0;

		return PTRUE;
	}

	case CO_OPERATION_PRINTK: {
		unsigned long size = co_passage_page->params[0];
		char *ptr = (char *)&co_passage_page->params[1];
		co_message_t *co_message;

		if (size > 200) 
			size = 200; /* sanity, see co_terminal_printv */

		co_message = co_os_malloc(1 + size + sizeof(*co_message));
		if (co_message) {
			co_message->from = CO_MODULE_LINUX;
			co_message->to = CO_MODULE_PRINTK;
			co_message->priority = CO_PRIORITY_DISCARDABLE;
			co_message->type = CO_MESSAGE_TYPE_STRING;
			co_message->size = size + 1; 
			co_memcpy(co_message->data, ptr, size + 1);
			incoming_message(cmon, co_message);
			co_os_free(co_message);
		}

		return PTRUE;
	}

	case CO_OPERATION_DEVICE: {
		unsigned long device = co_passage_page->params[0];
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_DEVICE)\n");
		return device_request(cmon, device, &co_passage_page->params[1]);
	}
	case CO_OPERATION_GET_TIME: {
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_GET_TIME)\n");
		co_passage_page->params[0] = co_os_get_time();
		return PTRUE;
	}
	case CO_OPERATION_GET_HIGH_PREC_TIME: {
		co_os_get_timestamp((co_timestamp_t *)&co_passage_page->params[0]);
		co_os_get_timestamp_freq((co_timestamp_t *)&co_passage_page->params[2]);
		return PTRUE;
	}

        case CO_OPERATION_DEBUG_LINE: 
        case CO_OPERATION_TRACE_POINT: 
                return PTRUE;

	default:
		co_debug_lvl(context_switch, 5, "unknown operation %d not handled\n", co_passage_page->operation);
		return PFALSE;
	}
	
	return PTRUE;
}

static void free_file_blockdevice(co_monitor_t *cmon, co_block_dev_t *dev)
{
	co_monitor_file_block_dev_t *fdev = (co_monitor_file_block_dev_t *)dev;

	co_monitor_file_block_shutdown(fdev);
	co_monitor_free(cmon, dev);
}

static co_rc_t load_configuration(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK; 
	long i;

	for (i=0; i < CO_MODULE_MAX_COBD; i++) {
		co_monitor_file_block_dev_t *dev;
		co_block_dev_desc_t *conf_dev = &cmon->config.block_devs[i];
		if (!conf_dev->enabled)
			continue;

		rc = co_monitor_malloc(cmon, sizeof(co_monitor_file_block_dev_t), (void **)&dev);
		if (!CO_OK(rc))
			goto error_1;

		rc = co_monitor_file_block_init(dev, &conf_dev->pathname);
		if (CO_OK(rc)) {
			dev->dev.conf = conf_dev;
			co_debug("cobd%d: enabled (%x)\n", i, dev);
			co_monitor_block_register_device(cmon, i, (co_block_dev_t *)dev);
			dev->dev.free = free_file_blockdevice;
		} else {
			co_monitor_free(cmon, dev);
			co_debug_error("cobd%d: cannot enable %d (%x)\n", i, rc);
			goto error_1;
		}
	}

	for (i=0; i < CO_MODULE_MAX_COFS; i++) {
		co_cofsdev_desc_t *desc = &cmon->config.cofs_devs[i];
		if (!desc->enabled)
			continue;

		rc = co_monitor_file_system_init(cmon, i, desc);
		if (!CO_OK(rc))
			goto error_2;

	}

	return rc;

error_2:
	co_monitor_unregister_filesystems(cmon);
error_1:
	co_monitor_unregister_and_free_block_devices(cmon);

	return rc;
}

static void timer_callback(void *data)
{
        co_monitor_t *cmon = (co_monitor_t *)data;

        cmon->timer_interrupt = PTRUE;

	co_os_wait_wakeup(cmon->idle_wait);
}

static void free_pseudo_physical_memory(co_monitor_t *monitor)
{
	int i, j;

	if (!monitor->pp_pfns) 
		return;

	co_debug("freeing page frames for pseudo physical RAM\n");

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

	co_debug("done freeing\n");
}

static co_rc_t alloc_pp_ram_mapping(co_monitor_t *monitor)
{
	co_rc_t rc;
	unsigned long full_page_tables_size;
	unsigned long partial_page_table_size;

	co_debug("allocating page frames for pseudo physical RAM...\n");

	rc = co_monitor_malloc(monitor, sizeof(co_pfn_t *)*PTRS_PER_PGD, (void **)&monitor->pp_pfns);
	if (!CO_OK(rc))
		return rc;

	co_memset(monitor->pp_pfns, 0, sizeof(co_pfn_t *)*PTRS_PER_PGD);

	full_page_tables_size = CO_ARCH_PAGE_SIZE * (monitor->memory_size >> CO_ARCH_PMD_SHIFT);
	partial_page_table_size = sizeof(unsigned long) *
		((monitor->memory_size & ~CO_ARCH_PMD_MASK) >> (CO_ARCH_PAGE_SHIFT));

	rc = co_monitor_scan_and_create_pfns(
		monitor, 
		CO_VPTR_PSEUDO_RAM_PAGE_TABLES, 
		full_page_tables_size);

	if (CO_OK(rc)) {
		if (partial_page_table_size) {
			rc = co_monitor_scan_and_create_pfns(
				monitor, 
				CO_VPTR_PSEUDO_RAM_PAGE_TABLES + full_page_tables_size, 
				partial_page_table_size);
		}
	}
	
	if (!CO_OK(rc)) {
		free_pseudo_physical_memory(monitor);
	}

	return rc;
}

static co_rc_t alloc_shared_page(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC_OK;
	
	cmon->shared = co_os_alloc_pages(1);
	if (!cmon->shared)
		return CO_RC(ERROR);

	rc = co_os_userspace_map(cmon->shared, 1, &cmon->shared_user_address, &cmon->shared_handle);
	if (!CO_OK(rc)) {
		co_os_free_pages(cmon->shared, 1);
		return rc;
	}

	return rc;
}

static void free_shared_page(co_monitor_t *cmon)
{
	if (cmon->shared_user_address) {
		co_os_userspace_unmap(cmon->shared_user_address, cmon->shared_handle, 1);
	}
	co_os_free_pages(cmon->shared, 1);
}

static co_rc_t load_section(co_monitor_t *cmon, co_monitor_ioctl_load_section_t *params)
{
	co_rc_t rc = CO_RC(OK);

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED)
		return CO_RC(ERROR);

	if (params->user_ptr) {
		co_debug("loading section at 0x%x (0x%x bytes)\n", params->address, params->size);
		rc = co_monitor_copy_region(cmon, params->address, params->size, params->buf);
	} else {
		rc = co_monitor_copy_region(cmon, params->address, params->size, NULL);
	}

	return rc;
}

static co_rc_t load_initrd(co_monitor_t *cmon, co_monitor_ioctl_load_initrd_t *params)
{
	co_rc_t rc = CO_RC(OK);
	unsigned long address, pages;

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED)
		return CO_RC(ERROR);

	pages = ((params->size + CO_ARCH_PAGE_SIZE) >> CO_ARCH_PAGE_SHIFT);

        /*
	 * Put initrd at the end of the address space.
	 */
	address = CO_ARCH_KERNEL_OFFSET + cmon->memory_size - (pages << CO_ARCH_PAGE_SHIFT);
	
	co_debug("initrd address: %x (0x%x pages)\n", address, pages);

	if (address <= cmon->core_end + 0x100000) {
		co_debug_error("initrd is too close to the kernel code, not enough memory)\n");

		/* We are too close to the kernel code, not enough memory */
		return CO_RC(ERROR);
	}

	rc = co_monitor_copy_region(cmon, address, params->size, params->buf);
	if (!CO_OK(rc)) {
		co_debug_error("initrd copy failed (%x)\n", rc);
		return rc;
	}

	cmon->initrd_address = address;
	cmon->initrd_size = params->size;

	return rc;
}


static co_rc_t start(co_monitor_t *cmon)
{
	co_rc_t rc;

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED) {
		co_debug_error("invalid state\n");
		return CO_RC(ERROR);
	}
		
	rc = guest_address_space_init(cmon);
	if (!CO_OK(rc)) {
		co_debug_error("error initializing coLinux context (%d)\n", rc);
		return rc;
	}

	co_os_get_timestamp(&cmon->timestamp);
	co_os_get_timestamp_freq(&cmon->timestamp_freq);

	co_os_timer_activate(cmon->timer);

	co_passage_page->operation = CO_OPERATION_START;
	co_passage_page->params[0] = cmon->core_end;
	co_passage_page->params[1] = cmon->memory_size;
	co_passage_page->params[2] = cmon->initrd_address;
	co_passage_page->params[3] = cmon->initrd_size;

	co_memcpy(&co_passage_page->params[10], cmon->config.boot_parameters_line, 
	       sizeof(cmon->config.boot_parameters_line));

	cmon->state = CO_MONITOR_STATE_STARTED;

	return CO_RC(OK);
}

static co_rc_t run(co_monitor_t *cmon,
		   co_monitor_ioctl_run_t *params,
		   unsigned long out_size,
		   unsigned long *return_size)
{
	*return_size = sizeof(*params);

	if (cmon->state == CO_MONITOR_STATE_RUNNING) {
		bool_t ret;
		do {
			ret = iteration(cmon);
		} while (ret);
		return CO_RC(OK);
	} else if (cmon->state == CO_MONITOR_STATE_STARTED) {
		cmon->state = CO_MONITOR_STATE_RUNNING;
		iteration(cmon);
		return CO_RC(OK);
	}
	
	if (cmon->state == CO_MONITOR_STATE_TERMINATED)
		return CO_RC(INSTANCE_TERMINATED);

	return CO_RC(ERROR);
}


co_rc_t co_monitor_create(co_manager_t *manager, co_manager_ioctl_create_t *params, co_monitor_t **cmon_out)
{
	co_symbols_import_t *import = &params->import;
	co_monitor_t *cmon;
	co_rc_t rc = CO_RC_OK;

	cmon = co_os_malloc(sizeof(*cmon));
	if (!cmon) {
		rc = CO_RC(OUT_OF_MEMORY);
		goto out;
	}

	co_memset(cmon, 0, sizeof(*cmon));

	cmon->manager = manager;
	cmon->state = CO_MONITOR_STATE_EMPTY;
	cmon->id = co_os_current_id();

	rc = co_console_create(80, 25, 0, &cmon->console);
	if (!CO_OK(rc))
		goto out_free_monitor;
	
	rc = co_os_mutex_create(&cmon->connected_modules_write_lock);
	if (!CO_OK(rc))
		goto out_free_console;

	rc = co_os_mutex_create(&cmon->linux_message_queue_mutex);
	if (!CO_OK(rc))
		goto out_free_mutex1;

	rc = co_os_wait_create(&cmon->idle_wait);
	if (!CO_OK(rc))
		goto out_free_mutex2;

	params->id = cmon->id;

	cmon->io_buffer = (co_io_buffer_t *)(co_os_malloc(CO_VPTR_IO_AREA_SIZE));
	if (cmon->io_buffer == NULL) {
		rc = CO_RC(OUT_OF_MEMORY);
		goto out_free_wait;
	}
	co_memset(cmon->io_buffer, 0, CO_VPTR_IO_AREA_SIZE);

	rc = alloc_shared_page(cmon);
	if (!CO_OK(rc))
		goto out_free_buffer;

	params->shared_user_address = cmon->shared_user_address;
	rc = co_queue_init(&cmon->linux_message_queue);
	if (!CO_OK(rc))
		goto out_free_shared_page;

	rc = co_monitor_os_init(cmon);
	if (!CO_OK(rc))
		goto out_free_linux_message_queue;

	cmon->core_vaddr = import->kernel_start;
	cmon->core_pages = (import->kernel_end - import->kernel_start + CO_ARCH_PAGE_SIZE - 1) >> CO_ARCH_PAGE_SHIFT;
	cmon->core_end = cmon->core_vaddr + (cmon->core_pages << CO_ARCH_PAGE_SHIFT);
	cmon->import = params->import;
	cmon->config = params->config;
	cmon->info = params->info;
	cmon->arch_info = params->arch_info;

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

		if (cmon->manager->hostmem_amount >= 128*1024*1024) {
			/* Use quarter */
			cmon->memory_size = cmon->manager->hostmem_amount/(1024*1024*4);
		} else {
			cmon->memory_size = 16;
		}
	} else {
		cmon->memory_size = cmon->config.ram_size;
	}

	co_debug("configured to %d MB\n", cmon->memory_size);

	if (cmon->memory_size < 8)
		cmon->memory_size = 8;
	else if (cmon->memory_size > 1000)	/* 1000 = 1024 - 8*4 */
		cmon->memory_size = 1000;	/* 24MB = 8 Pages a 4K reserved */

	co_debug("after adjustments: %d MB\n", cmon->memory_size);

	cmon->memory_size <<= 20; /* Megify */
	
	if (cmon->manager->hostmem_used + cmon->memory_size > cmon->manager->hostmem_usage_limit) {
		rc = CO_RC(HOSTMEM_USE_LIMIT_REACHED);
		params->actual_memsize_used = cmon->memory_size;
		goto out_free_os_dep;
	}

	cmon->manager->hostmem_used += cmon->memory_size;

	cmon->physical_frames = cmon->memory_size >> CO_ARCH_PAGE_SHIFT;
	cmon->end_physical = CO_ARCH_KERNEL_OFFSET + cmon->memory_size;
	cmon->passage_page_vaddr = CO_VPTR_PASSAGE_PAGE;

	rc = alloc_pp_ram_mapping(cmon);
        if (!CO_OK(rc)) {
		goto out_revert_used_mem;
	}

        rc = co_os_timer_create(&timer_callback, cmon, 10, &cmon->timer);
        if (!CO_OK(rc)) {
                co_debug_error("creating host OS timer (%x)\n", rc);
                goto out_free_pp;
        }

	rc = load_configuration(cmon);
	if (!CO_OK(rc)) {
		co_debug_error("loading monitor configuration (%x)\n", rc);
		goto out_destroy_timer;
	}

	co_os_mutex_acquire(manager->lock);
	cmon->refcount = 1;
	manager->monitors_count++;
	cmon->listed_in_manager = PTRUE;
	co_list_add_head(&cmon->node, &manager->monitors);
	co_os_mutex_release(manager->lock);

	cmon->state = CO_MONITOR_STATE_INITIALIZED;

	*cmon_out = cmon;

	return CO_RC(OK);

/* error path */
out_destroy_timer:
        co_os_timer_destroy(cmon->timer);

out_free_pp:
	free_pseudo_physical_memory(cmon);

out_revert_used_mem:
	cmon->manager->hostmem_used -= cmon->memory_size;

out_free_os_dep:
	co_monitor_os_exit(cmon);

out_free_linux_message_queue:
	co_queue_flush(&cmon->linux_message_queue);

out_free_shared_page:
	free_shared_page(cmon);

out_free_buffer:
	co_os_free(cmon->io_buffer);

out_free_wait:
	co_os_wait_destroy(cmon->idle_wait);

out_free_mutex2:
	co_os_mutex_destroy(cmon->connected_modules_write_lock);

out_free_mutex1:
	co_os_mutex_destroy(cmon->linux_message_queue_mutex);

out_free_console:
	co_console_destroy(cmon->console);

out_free_monitor:
	co_os_free(cmon);

out:
	return rc;
}


static co_rc_t co_monitor_destroy(co_monitor_t *cmon, bool_t user_context)
{
	co_manager_t *manager;

	manager = cmon->manager;

	co_debug("cleaning up\n");
	co_debug("before free: %d blocks\n", cmon->blocks_allocated);

	if (cmon->state == CO_MONITOR_STATE_RUNNING ||
	    cmon->state == CO_MONITOR_STATE_STARTED)
	{
                co_os_timer_deactivate(cmon->timer);
	}

	if (!user_context)
		cmon->shared_user_address = NULL;

	co_monitor_unregister_and_free_block_devices(cmon);
	co_monitor_unregister_filesystems(cmon);
	free_pseudo_physical_memory(cmon);
	manager->hostmem_used -= cmon->memory_size;
	co_os_free(cmon->io_buffer);
	free_shared_page(cmon);
	co_monitor_os_exit(cmon);
	co_queue_flush(&cmon->linux_message_queue);
        co_os_timer_destroy(cmon->timer);
	co_os_mutex_destroy(cmon->connected_modules_write_lock);
	co_os_mutex_destroy(cmon->linux_message_queue_mutex);
	co_console_destroy(cmon->console);
	cmon->console = NULL;

	co_debug("after free: %d blocks\n", cmon->blocks_allocated);
	co_os_free(cmon);

	return CO_RC_OK;
}

static void send_monitor_end_messages(co_monitor_t *cmon)
{
	co_manager_open_desc_t opened;
	int i;
	
	co_os_mutex_acquire(cmon->connected_modules_write_lock);
	for (i = 0; i <  CO_MONITOR_MODULES_COUNT; i++) {
		opened = cmon->connected_modules[i];
		if (!opened)
			continue;

		co_manager_send_eof(cmon->manager, opened);
		cmon->connected_modules[i] = NULL;
		co_manager_close(cmon->manager, opened);
	}
	co_os_mutex_release(cmon->connected_modules_write_lock);
}

co_rc_t co_monitor_refdown(co_monitor_t *cmon, bool_t user_context, bool_t monitor_owner)
{
	co_manager_t *manager;
	int new_count;
	bool_t destroy = PFALSE;
	bool_t end_messages = PFALSE;

	manager = cmon->manager;

	co_os_mutex_acquire(manager->lock);
	new_count = cmon->refcount;
	if (new_count > 0) {
		cmon->refcount--;

		if (monitor_owner  &&  cmon->listed_in_manager) {
			cmon->listed_in_manager = PFALSE;
			manager->monitors_count--;
			co_list_del(&cmon->node);
		}

		if (monitor_owner)
			end_messages = PTRUE;

		if (cmon->refcount == 0) {
			destroy = PTRUE;
		}
	}
	co_os_mutex_release(manager->lock);

	if (end_messages)
		send_monitor_end_messages(cmon);

	if (destroy) {
		return co_monitor_destroy(cmon, monitor_owner);
	}

	return CO_RC(OK);
}

static co_rc_t co_monitor_user_reset(co_monitor_t *monitor)
{
	co_rc_t rc;

	monitor->state = CO_MONITOR_STATE_EMPTY;

	co_os_mutex_acquire(monitor->linux_message_queue_mutex);
	co_queue_flush(&monitor->linux_message_queue);
	co_os_mutex_release(monitor->linux_message_queue_mutex);

	free_pseudo_physical_memory(monitor);
	rc = alloc_pp_ram_mapping(monitor);
	if (!CO_OK(rc))
		goto out;

	co_monitor_unregister_and_free_block_devices(monitor);
	co_monitor_unregister_filesystems(monitor);

	rc = load_configuration(monitor);
	if (!CO_OK(rc)) {
		free_pseudo_physical_memory(monitor);
		goto out; 
	}

	co_os_mutex_acquire(monitor->manager->lock);
	if (!monitor->listed_in_manager) {
		monitor->listed_in_manager = PTRUE;
		co_list_add_head(&monitor->node, &monitor->manager->monitors);
	}
	co_os_mutex_release(monitor->manager->lock);

	co_memset(monitor->io_buffer, 0, CO_VPTR_IO_AREA_SIZE);

	monitor->state = CO_MONITOR_STATE_INITIALIZED;
	monitor->termination_reason = CO_TERMINATE_END;

out:	
	return rc;
}

static co_rc_t co_monitor_user_get_state(co_monitor_t *monitor, co_monitor_ioctl_get_state_t *params)
{
	params->monitor_state = monitor->state;
	params->termination_reason = monitor->termination_reason;
	params->bug_info = monitor->bug_info;

	return CO_RC(OK);
}

static co_rc_t co_monitor_user_get_console(co_monitor_t *monitor, co_monitor_ioctl_get_console_t *params)
{
	co_message_t *co_message = NULL;
	co_console_message_t *message = NULL;
	unsigned long size;
	int y;

	message = NULL;
	size = (((char *)(&message->putcs + 1)) - ((char *)message)) + 
		(monitor->console->x * sizeof(co_console_cell_t));

	params->x = monitor->console->x;
	params->y = monitor->console->y;
							  
	co_message = co_os_malloc(size + sizeof(*co_message));
	if (!co_message)
		return CO_RC(OUT_OF_MEMORY);

	message = (co_console_message_t *)co_message->data;
	co_message->from = CO_MODULE_LINUX;
	co_message->to = CO_MODULE_CONSOLE;
	co_message->priority = CO_PRIORITY_DISCARDABLE;
	co_message->type = CO_MESSAGE_TYPE_STRING;
	co_message->size = size; 
	message->type = CO_OPERATION_CONSOLE_PUTCS;
	message->putcs.x = 0;
	message->putcs.count = monitor->console->x;

	for (y=0; y < monitor->console->y; y++) {
		co_memcpy(&message->putcs.data, 
			  &monitor->console->screen[y*monitor->console->x], 
			  monitor->console->x * sizeof(unsigned short));
		message->putcs.y = y;
		incoming_message(monitor, co_message);
	}

	co_os_free(co_message);

	return CO_RC(OK);
}

co_rc_t co_monitor_ioctl(co_monitor_t *cmon, co_manager_ioctl_monitor_t *io_buffer,
			 unsigned long in_size, unsigned long out_size, 
			 unsigned long *return_size, co_manager_open_desc_t opened_manager)
{
	co_rc_t rc = CO_RC_ERROR;

	switch (io_buffer->op) {
	case CO_MONITOR_IOCTL_CLOSE: {
		co_monitor_refdown(cmon, PTRUE, opened_manager->monitor_owner);
		opened_manager->monitor = NULL;
		return CO_RC_OK;
	}
	case CO_MONITOR_IOCTL_GET_CONSOLE: {
		co_monitor_ioctl_get_console_t *params;

		*return_size = sizeof(*params);
		params = (typeof(params))(io_buffer);

		return co_monitor_user_get_console(cmon, params);
	}
	default:
		break;
	}

	if (!opened_manager->monitor_owner)
		return rc;

	switch (io_buffer->op) {
	case CO_MONITOR_IOCTL_GET_STATE: {
		co_monitor_ioctl_get_state_t *params;

		*return_size = sizeof(*params);
		params = (typeof(params))(io_buffer);

		return co_monitor_user_get_state(cmon, params);
	}
	case CO_MONITOR_IOCTL_RESET: {
		return co_monitor_user_reset(cmon);
	}
	case CO_MONITOR_IOCTL_LOAD_SECTION: {
		return load_section(cmon, (co_monitor_ioctl_load_section_t *)io_buffer);
	}
	case CO_MONITOR_IOCTL_LOAD_INITRD: {
		return load_initrd(cmon, (co_monitor_ioctl_load_initrd_t *)io_buffer);
	}
	case CO_MONITOR_IOCTL_START: {
		return start(cmon);
	}
	case CO_MONITOR_IOCTL_RUN: {
		return run(cmon, (co_monitor_ioctl_run_t *)io_buffer, out_size, return_size);
	}
	default:
		return rc;
	}

	return rc;
}
