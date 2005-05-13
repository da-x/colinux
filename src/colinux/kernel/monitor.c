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

#if (0)
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
	co_os_unmap(cmon->manager, page, pfn);

	return CO_RC(OK);
}

static co_rc_t colinux_dump_page_at_address(co_monitor_t *cmon, vm_ptr_t address)
{
	co_rc_t rc;
	co_pfn_t pfn = 0;

	co_debug("dump of address %08x\n", address);
	rc = co_monitor_get_pfn(cmon, address, &pfn);
	if (!CO_OK(rc))
		return rc;

	return colinux_dump_page_at_pfn(cmon, pfn);
}
#endif

static co_rc_t guest_address_space_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC(OK);
	co_pfn_t *pfns = NULL, self_map_pfn, passage_page_pfn, swapper_pg_dir_pfn;
	unsigned long pp_pagetables_pgd, self_map_page_offset, passage_page_offset;
	unsigned long reversed_physical_mapping_offset;

	rc = co_monitor_get_pfn(cmon, cmon->import.kernel_swapper_pg_dir, &swapper_pg_dir_pfn);
	if (!CO_OK(rc)) {
		co_debug("error getting swapper_pg_dir pfn (%x)\n", rc);
		goto out_error;
	}	
	
	cmon->pgd = swapper_pg_dir_pfn << CO_ARCH_PAGE_SHIFT;

	rc = co_monitor_arch_passage_page_alloc(cmon);
	if (!CO_OK(rc)) {
		co_debug("error allocating passage page (%d)\n", rc);
		goto out_error;
	}

	rc = co_monitor_arch_passage_page_init(cmon);
	if (!CO_OK(rc)) {
		co_debug("error initializing the passage page (%d)\n", rc);
		goto out_error;
	}

	pp_pagetables_pgd = CO_VPTR_PSEUDO_RAM_PAGE_TABLES >> PGDIR_SHIFT;
	pfns = cmon->pp_pfns[pp_pagetables_pgd];
	if (pfns == NULL) {
		co_debug("CO_VPTR_PSEUDO_RAM_PAGE_TABLES is not mapped, huh? (%x)\n");
		goto out_error;
	}

	rc = co_monitor_create_ptes(cmon, cmon->import.kernel_swapper_pg_dir, CO_ARCH_PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug("error initializing swapper_pg_dir (%x)\n", rc);
		goto out_error;
	}

	reversed_physical_mapping_offset = (CO_VPTR_PHYSICAL_TO_PSEUDO_PFN_MAP >> PGDIR_SHIFT)*sizeof(linux_pgd_t);

	rc = co_monitor_copy_and_create_pfns(cmon, 
					     cmon->import.kernel_swapper_pg_dir + reversed_physical_mapping_offset, 
					     sizeof(linux_pgd_t)*cmon->manager->reversed_map_pgds_count, 
					     (void *)cmon->manager->reversed_map_pgds);
	if (!CO_OK(rc)) {
		co_debug("error adding reversed physical mapping (%x)\n", rc);
		goto out_error;
	}	

	rc = co_monitor_create_ptes(cmon, CO_VPTR_SELF_MAP, CO_ARCH_PAGE_SIZE, pfns);
	if (!CO_OK(rc)) {
		co_debug("error initializing self_map (%x)\n", rc);
		goto out_error;
	}

	rc = co_monitor_get_pfn(cmon, CO_VPTR_SELF_MAP, &self_map_pfn);
	if (!CO_OK(rc)) {
		co_debug("error getting self_map pfn (%x)\n", rc);
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
		co_debug("error getting self_map pfn (%x)\n", rc);
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
		co_debug("error mapping passage page into the self map (%x)\n", rc);
		goto out_error;
	}	

	rc = co_monitor_create_ptes(
		cmon, 
		CO_VPTR_SELF_MAP, 
		sizeof(linux_pte_t), 
		&self_map_pfn);
	
	if (!CO_OK(rc)) {
		co_debug("error initializing self_map (%x)\n", rc);
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
			co_debug("error initializing io buffer (%x, %d)\n", rc, io_buffer_page);
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

	co_passage_page->params[0] = 0;

	if (!cmon->io_buffer)
		return CO_RC(ERROR);

	cmon->io_buffer->messages_waiting = 0;

	io_buffer = cmon->io_buffer->buffer;
	io_buffer_end = io_buffer + CO_VPTR_IO_AREA_SIZE - sizeof(co_io_buffer_t);

	cmon->io_buffer->messages_waiting = 0;
	
	co_queue_t *queue = &cmon->linux_message_queue;
	while (co_queue_size(queue) != 0) 
	{
		co_message_queue_item_t *message_item;
		rc = co_queue_peek_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			return rc;
		
		co_message_t *message = message_item->message;
		unsigned long size = message->size + sizeof(*message);
		
		if (io_buffer + size > io_buffer_end) {
			break;
		}
		
		rc = co_queue_pop_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			return rc;

		cmon->io_buffer->messages_waiting += 1;
		co_queue_free(queue, message_item);
		co_memcpy(io_buffer, message, size);
		io_buffer += size;
		co_os_free(message);
	}

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

void co_monitor_debug_line(co_monitor_t *cmon, const char *text)
{
	struct {
		co_message_t message;
		co_daemon_message_t payload;
		char data[0];
	} *message = NULL;
	unsigned long size;

	size = sizeof(message->payload) + co_strlen(text) + 1;
	message = (typeof(message))co_os_malloc(size + sizeof(message->message));
	if (message == NULL)
		return;

	message->message.from = CO_MODULE_MONITOR;
	message->message.to = CO_MODULE_DAEMON;
	message->message.priority = CO_PRIORITY_IMPORTANT;
	message->message.type = CO_MESSAGE_TYPE_STRING;
	message->message.size = size;
	message->payload.type = CO_MONITOR_MESSAGE_TYPE_DEBUG_LINE;
	co_memcpy(message->data, text, co_strlen(text)+1);

	co_message_switch_message(&cmon->message_switch, &message->message);
}

void co_monitor_trace_point_info(co_monitor_t *cmon, co_trace_point_info_t *info)
{
	struct {
		co_message_t message;
		co_daemon_message_t payload;
		co_trace_point_info_t data;
	} *message = NULL;
	unsigned long size;

	size = sizeof(message->payload) + sizeof(message->data);
	message = (typeof(message))co_os_malloc(size + sizeof(message->message));
	if (message == NULL)
		return;

	message->message.from = CO_MODULE_MONITOR;
	message->message.to = CO_MODULE_DAEMON;
	message->message.priority = CO_PRIORITY_DISCARDABLE;
	message->message.type = CO_MESSAGE_TYPE_OTHER;
	message->message.size = size;
	message->payload.type = CO_MONITOR_MESSAGE_TYPE_TRACE_POINT;
	message->data = *info;

	co_message_switch_message(&cmon->message_switch, &message->message);
}

bool_t co_monitor_trace_point(co_monitor_t *cmon)
{
	co_trace_point_info_t *info = ((co_trace_point_info_t *)&(co_passage_page->params[0]));

	if (info->index < 3000000)
		return PTRUE;

	co_monitor_trace_point_info(cmon, info);

	return PFALSE;
}


static bool_t co_terminate(co_monitor_t *cmon)
{
	struct {
		co_message_t message;
		co_daemon_message_t payload;
	} message;
		
	co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_TERMINATE)\n");
	co_debug("linux terminated (%d)\n", co_passage_page->params[0]);

	/* Prints kernel BUG */
	if (co_passage_page->params[0] == CO_TERMINATE_BUG)
	{
		unsigned int len = co_passage_page->params[3];
		
		if (len > 255)
			len = 255; /* sanity */

		char file_name [len+1]; /* Buffer on stack */
			
		co_memcpy (file_name, &co_passage_page->params[4], len);
		file_name [len] = 0;
		co_debug("%s(%d): Kernel BUG %d\n",
			file_name, co_passage_page->params[2], co_passage_page->params[1]);
	}
		
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

static bool_t co_idle(co_monitor_t *cmon)
{
	co_message_t message;

	co_debug_lvl(context_switch, 15, "switching from linux (CO_OPERATION_IDLE)\n");
		
	message.from = CO_MODULE_MONITOR;
	message.to = CO_MODULE_IDLE;
	message.priority = CO_PRIORITY_DISCARDABLE;
	message.type = CO_MESSAGE_TYPE_STRING;
	message.size = 0;

	co_message_switch_dup_message(&cmon->message_switch, &message);

	return PFALSE;
}

static void co_free_pages(co_monitor_t *cmon, vm_ptr_t address, int num_pages)
{
	//co_debug_system("free_pages: %x %x\n", address, num_pages);
#if (1)
	unsigned long scan_address;
	int j;

	scan_address = address;
	for (j=0; j < num_pages; j++) {
		co_monitor_free_and_unmap_page(cmon, scan_address);
		scan_address += CO_ARCH_PAGE_SIZE;
	}
#endif
}

static co_rc_t co_alloc_pages(co_monitor_t *cmon, vm_ptr_t address, int num_pages)
{
#if (1)
	//co_debug_system("alloc_pages: %x %x\n", address, num_pages);
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

	return rc;
#else
	return CO_RC(OK);
#endif
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
			return PTRUE;
		}
		return PFALSE;
	}
	case CO_OPERATION_TERMINATE: 
		return co_terminate(cmon);

	case CO_OPERATION_IDLE:
		return co_idle(cmon);

        case CO_OPERATION_DEBUG_LINE: {
		char *p = (char *)&co_passage_page->params[0];
		p[0x200] = '\0';
		co_monitor_debug_line(cmon, p);
                return PFALSE;
        }

        case CO_OPERATION_TRACE_POINT: {
		return co_monitor_trace_point(cmon);
        }

	case CO_OPERATION_MESSAGE_TO_MONITOR: {
		co_message_t *message;
		co_rc_t rc;
		
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_MESSAGE_TO_MONITOR)\n");

		message = (co_message_t *)cmon->io_buffer->buffer;
		if (message && message->to < CO_MAX_MONITORS)
			rc = co_message_switch_dup_message(&cmon->message_switch, message);

		cmon->io_buffer->messages_waiting = 0;

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
	case CO_OPERATION_PRINTK: {
		unsigned long size = co_passage_page->params[0];
		if (size > 255) 
			size = 255; /* sanity */

		char bf [size+1]; /* Buffer on stack */

		co_memcpy (bf, &co_passage_page->params[1], size);
		bf [size] = 0;
		co_debug("%s", bf);
		return PFALSE;
	}


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
			co_debug("cobd%d: cannot enable %d (%x)\n", i, rc);
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
}

static void free_pseudo_physical_memory(co_monitor_t *monitor)
{
	int i, j;

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
                                  ((monitor->memory_size & ~CO_ARCH_PMD_MASK) >> CO_ARCH_PAGE_SHIFT);

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
	co_os_userspace_unmap(cmon->shared_user_address, cmon->shared_handle, 1);
	co_os_free_pages(cmon->shared, 1);
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

	co_memset(cmon, 0, sizeof(*cmon));
	cmon->manager = manager;
	cmon->state = CO_MONITOR_STATE_INITIALIZED;
	cmon->console_state = CO_MONITOR_CONSOLE_STATE_DETACHED;
	cmon->io_buffer = (co_io_buffer_t *)(co_os_malloc(CO_VPTR_IO_AREA_SIZE));
	if (cmon->io_buffer == NULL)
		goto out_free_monitor;
	co_memset(cmon->io_buffer, 0, CO_VPTR_IO_AREA_SIZE);

	rc = alloc_shared_page(cmon);
	if (!CO_OK(rc))
		goto out_free_buffer;

	params->shared_user_address = cmon->shared_user_address;
	co_message_switch_init(&cmon->message_switch, CO_MODULE_KERNEL_SWITCH);

	rc = co_queue_init(&cmon->user_message_queue);
	if (!CO_OK(rc))
		goto out_free_shared_page;

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
	else if (cmon->memory_size > 1000)		/* 1000 = 1024 - 8*4 */
		cmon->memory_size = 1000;		/* 24 MB = 8 Pages a 4 K reserved */

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
                co_debug("creating host OS timer (%d)\n", rc);
                goto out_free_pp;
        }

	rc = load_configuration(cmon);
	if (!CO_OK(rc)) {
		co_debug("loading monitor configuration (%d)\n", rc);
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

out_revert_used_mem:
	cmon->manager->hostmem_used -= cmon->memory_size;

out_free_os_dep:
	co_monitor_os_exit(cmon);

out_free_linux_message_queue:
	co_queue_flush(&cmon->linux_message_queue);

out_free_user_message_queue:
	co_queue_flush(&cmon->user_message_queue);

out_free_shared_page:
	free_shared_page(cmon);
	co_message_switch_free(&cmon->message_switch);

out_free_buffer:
	co_os_free(cmon->io_buffer);

out_free_monitor:
	co_os_free(cmon);

out:
	manager->monitors_count--;
	return rc;
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
		co_debug("initrd is too close to the kernel code, not enough memory)\n");

		/* We are too close to the kernel code, not enough memory */
		return CO_RC(ERROR);
	}

	rc = co_monitor_copy_region(cmon, address, params->size, params->buf);
	if (!CO_OK(rc)) {
		co_debug("initrd copy failed (%x)\n", rc);
		return rc;
	}

	cmon->initrd_address = address;
	cmon->initrd_size = params->size;

	return rc;
}

co_rc_t co_monitor_destroy(co_monitor_t *cmon, bool_t user_context)
{
	co_manager_t *manager;
	
	co_debug("cleaning up\n");
	co_debug("before free: %d blocks\n", cmon->blocks_allocated);

	manager = cmon->manager;

	if (cmon->state >= CO_MONITOR_STATE_RUNNING)
                co_os_timer_deactivate(cmon->timer);

	if (!user_context)
		cmon->shared_user_address = NULL;

	co_monitor_unregister_and_free_block_devices(cmon);
	free_pseudo_physical_memory(cmon);
	manager->hostmem_used -= cmon->memory_size;
	co_os_free(cmon->io_buffer);
	free_shared_page(cmon);
	co_monitor_os_exit(cmon);
	co_queue_flush(&cmon->linux_message_queue);
	co_queue_flush(&cmon->user_message_queue);
	co_message_switch_free(&cmon->message_switch);
        co_os_timer_destroy(cmon->timer);

	co_debug("after free: %d blocks\n", cmon->blocks_allocated);

	co_os_free(cmon);

	manager->monitors_count--;

	return CO_RC_OK;
}

static co_rc_t start(co_monitor_t *cmon)
{
	co_rc_t rc;

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED) {
		co_debug("invalid state\n");
		return CO_RC(ERROR);
	}
		
	rc = guest_address_space_init(cmon);
	if (!CO_OK(rc)) {
		co_debug("error initializing coLinux context (%d)\n", rc);
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

	cmon->state = CO_MONITOR_STATE_RUNNING;

	return CO_RC(OK);
}

static co_rc_t run(co_monitor_t *cmon,
		   co_monitor_ioctl_run_t *params,
		   unsigned long out_size,
		   unsigned long *return_size)
{
	char *params_data;
	int i;

	co_debug_lvl(context_switch, 13, "running with %d messages\n", params->num_messages);

	co_monitor_os_before_iterations(cmon);

	/* Get messages from userspace */
	params_data = params->data;

	for (i=0; i < params->num_messages; i++) {
		co_message_t *message = (typeof(message))params_data;

		co_message_switch_dup_message(&cmon->message_switch, message);
		
		params_data += message->size + sizeof(*message);
	}

	if (cmon->state == CO_MONITOR_STATE_RUNNING) {
		bool_t ret;
		do {
			ret = iteration(cmon);
		} while (ret);
	}

	/* Flush messages to userspace */
	*return_size = sizeof(*params);
	params->num_messages = 0;

	int num_messages = co_queue_size(&cmon->user_message_queue);

	co_debug_lvl(context_switch, 13, "returns with %d messages\n", num_messages);

	co_monitor_os_after_iterations(cmon);

	if (num_messages == 0)
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
			 unsigned long *return_size, co_manager_per_fd_state_t *fd_state)
{
	co_rc_t rc = CO_RC_ERROR;

	switch (io_buffer->op) {
	case CO_MONITOR_IOCTL_DESTROY: {
		fd_state->monitor = NULL;
		co_monitor_destroy(cmon, PTRUE);
		return CO_RC_OK;
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

