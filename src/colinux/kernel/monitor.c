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
#include <colinux/os/current/kernel/main.h>
#include <colinux/arch/passage.h>
#include <colinux/arch/interrupt.h>
#include <colinux/arch/mmu.h>

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

	current_pfn = (address >> CO_ARCH_PAGE_SHIFT);
	pfn_group = current_pfn / PTRS_PER_PTE;
	pfn_index = current_pfn % PTRS_PER_PTE;

	if (cmon->pp_pfns[pfn_group] == NULL)
		return CO_RC(ERROR);

	*pfn = cmon->pp_pfns[pfn_group][pfn_index];

	return CO_RC(OK);;
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
	rc = colinux_get_pfn(cmon, address, &pfn);
	if (!CO_OK(rc))
		return rc;

	return colinux_dump_page_at_pfn(cmon, pfn);
}
#endif

static co_rc_t colinux_init(co_monitor_t *cmon)
{
	co_rc_t rc = CO_RC(OK);
	co_pfn_t *pfns = NULL, self_map_pfn, passage_page_pfn, swapper_pg_dir_pfn;
	unsigned long pp_pagetables_pgd, self_map_page_offset, passage_page_offset;
	unsigned long reversed_physical_mapping_offset;

	rc = colinux_get_pfn(cmon, cmon->import.kernel_swapper_pg_dir, &swapper_pg_dir_pfn);
	if (!CO_OK(rc)) {
		co_debug("error getting swapper_pg_dir pfn (%x)\n", rc);
		goto out_error;
	}	
	
	cmon->pgd = swapper_pg_dir_pfn << CO_ARCH_PAGE_SHIFT;
	co_debug_ulong(cmon->pgd);

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
	co_debug_ulong(reversed_physical_mapping_offset);

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
		co_debug("error initializing self map (%x)\n", rc);
		goto out_error;
	}

	rc = colinux_get_pfn(cmon, CO_VPTR_SELF_MAP, &self_map_pfn);
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

	for (io_buffer_page=0; io_buffer_page < io_buffer_num_pages; io_buffer_page++) {
		unsigned long io_buffer_pfn = co_os_virt_to_phys(&cmon->io_buffer[CO_ARCH_PAGE_SIZE*io_buffer_page]) >> CO_ARCH_PAGE_SHIFT;

		rc = co_monitor_create_ptes(cmon, CO_VPTR_SELF_MAP + io_buffer_offset, 
					    sizeof(linux_pte_t), &io_buffer_pfn);
		if (!CO_OK(rc)) {
			co_debug("error initializing io buffer (%x, %d)\n", rc, io_buffer_page);
			goto out_error;
		}	
		
		io_buffer_offset += sizeof(linux_pte_t);
	}

	co_debug("initialization finished\n");

out_error:
	return rc;
}

bool_t co_monitor_device_request(co_monitor_t *cmon, co_device_t device, unsigned long *params)
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
	default:
		break;
	}	

	return PTRUE;
}

static co_rc_t co_monitor_callback_return_messages(co_monitor_t *cmon)
{	
	co_rc_t rc;
	unsigned char *io_buffer, *io_buffer_end;

	io_buffer = cmon->io_buffer;
	if (!io_buffer)
		return CO_RC(ERROR);

	io_buffer_end = io_buffer + CO_VPTR_IO_AREA_SIZE;
	
	co_queue_t *queue = &cmon->linux_message_queue;
	while (co_queue_size(queue) != 0) 
	{
		co_message_queue_item_t *message_item;
		rc = co_queue_pop_tail(queue, (void **)&message_item);
		if (!CO_OK(rc))
			return rc;
		
		co_message_t *message = message_item->message;
		unsigned long size = message->size + sizeof(*message);
		co_queue_free(queue, message_item);
		
		if (io_buffer + size > io_buffer_end) {
			co_os_free(message);
			break;
		}
		
		co_memcpy(io_buffer, message, size);
		io_buffer += size;
		co_os_free(message);
	}

	co_passage_page->params[0] = io_buffer - cmon->io_buffer;

	co_debug_lvl(messages, 12, "sending messages to linux (%d bytes)\n", co_passage_page->params[0]);

	return CO_RC(OK);
}

static co_rc_t co_monitor_callback_return_jiffies(co_monitor_t *cmon)
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

static co_rc_t co_monitor_callback_return(co_monitor_t *cmon)
{
	co_passage_page->operation = CO_OPERATION_MESSAGE_FROM_MONITOR;

	co_monitor_callback_return_messages(cmon);
	co_monitor_callback_return_jiffies(cmon);

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
	co_memcpy(message->data, text, strlen(text)+1);

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

/*
 * co_monitor_iteration - returning PTRUE means that the driver will 
 * return immediately to Linux instead of returning to the host's 
 * userspace and only then to Linux.
 */

bool_t co_monitor_iteration(co_monitor_t *cmon)
{
	switch (co_passage_page->operation) {
	case CO_OPERATION_FORWARD_INTERRUPT: 
	case CO_OPERATION_IDLE: 
		co_monitor_callback_return(cmon);
		break;
	}

	co_debug_lvl(context_switch, 14, "switching to linux (%d)\n", co_passage_page->operation);
	co_host_switch_wrapper(cmon);
	if (co_passage_page->operation == CO_OPERATION_FORWARD_INTERRUPT)
		co_monitor_arch_real_hardware_interrupt(cmon);
	
	switch (co_passage_page->operation) {
	case CO_OPERATION_FORWARD_INTERRUPT: {
		co_debug_lvl(context_switch, 15, "switching from linux (CO_OPERATION_FORWARD_INTERRUPT), %d\n",
			     cmon->timer_interrupt);

		if (cmon->timer_interrupt) {
			cmon->timer_interrupt = PFALSE;
			return PTRUE;
		}
		return PFALSE;
	}
	case CO_OPERATION_TERMINATE: {
		struct {
			co_message_t message;
			co_daemon_message_t payload;
		} message;
		
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_TERMINATE)\n");
		co_debug("linux terminated (%d)\n", co_passage_page->params[0]);
		
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
	case CO_OPERATION_IDLE: {
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

		message = (co_message_t *)cmon->io_buffer;

		/* message = (co_message_t *)(&co_passage_page->params[0]); */

		rc = co_message_switch_dup_message(&cmon->message_switch, message);

		return PFALSE;
	}
	case CO_OPERATION_DEVICE: {
		unsigned long device = co_passage_page->params[0];

		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_DEVICE)\n");

		return co_monitor_device_request(cmon, device, &co_passage_page->params[1]);
	}
	case CO_OPERATION_GET_TIME: {
		co_debug_lvl(context_switch, 14, "switching from linux (CO_OPERATION_GET_TIME)\n");

		co_passage_page->params[0] = co_os_get_time();

		return PTRUE;
	}
	case CO_OPERATION_GET_HIGH_PREC_TIME: {
		co_os_get_timestamp(&co_passage_page->params[0]);
		co_os_get_timestamp_freq(&co_passage_page->params[2]);
		return PTRUE;
	}

	default:
		co_debug_lvl(context_switch, 5, "unknown operation %d not handled\n", co_passage_page->operation);
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

	for (i=0; i < CO_MODULE_MAX_COBD; i++) {
		co_monitor_file_block_dev_t *dev;
		co_block_dev_desc_t *conf_dev = &cmon->config.block_devs[i];
		if (!conf_dev->enabled)
			continue;

		rc = co_monitor_malloc(cmon, sizeof(co_monitor_file_block_dev_t), (void **)&dev);
		if (!CO_OK(rc))
			goto out;

		rc = co_monitor_file_block_init(dev, &conf_dev->pathname);
		if (CO_OK(rc)) {
			dev->dev.conf = conf_dev;
			co_debug("cobd%d: enabled (%x)\n", i, dev);
			co_monitor_block_register_device(cmon, i, (co_block_dev_t *)dev);
			dev->dev.free = co_monitor_free_file_blockdevice;
		} else {
			co_monitor_free(cmon, dev);
			co_debug("cobd%d: cannot enable %d (%x)\n", i, rc);
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

static co_rc_t alloc_pseudo_physical_memory(co_monitor_t *monitor)
{
	co_rc_t rc;
	int i, j;
	co_pfn_t pseudo_pfn = 0, first_pp_pgd;

	co_debug("allocating page frames for pseudo physical RAM...\n");

	rc = co_monitor_malloc(monitor, sizeof(co_pfn_t *)*PTRS_PER_PGD, (void **)&monitor->pp_pfns);
	if (!CO_OK(rc))
		return rc;

	co_memset(monitor->pp_pfns, 0, sizeof(co_pfn_t *)*PTRS_PER_PGD);

	rc = co_monitor_scan_and_create_pfns(monitor, CO_ARCH_KERNEL_OFFSET, monitor->physical_frames << CO_ARCH_PAGE_SHIFT);
	if (!CO_OK(rc)) {
		free_pseudo_physical_memory(monitor);
		return rc;
	}

	co_debug("setting reversed map\n");

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

	co_debug("creating page tables\n");

	first_pp_pgd = CO_ARCH_KERNEL_OFFSET >> PGDIR_SHIFT;
	for (i=first_pp_pgd; i < PTRS_PER_PGD; i++) {
		co_pfn_t *pfns = monitor->pp_pfns[i];
		unsigned long address;

		if (!pfns)
			break;

		address = CO_VPTR_PSEUDO_RAM_PAGE_TABLES + ((i - first_pp_pgd) * CO_ARCH_PAGE_SIZE);

		co_debug("creating one page table (at %x)\n", address);

		rc = co_monitor_create_ptes(monitor, address, CO_ARCH_PAGE_SIZE, pfns);
		if (!CO_OK(rc)) {
			free_pseudo_physical_memory(monitor);
			return rc;
		}
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
	cmon->io_buffer = co_os_malloc(CO_VPTR_IO_AREA_SIZE);
	if (cmon->io_buffer == NULL)
		goto out_free_monitor;

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

		if (cmon->manager->host_memory_amount >= 128*1024*1024) {
			/* Use quarter */
			cmon->memory_size = cmon->manager->host_memory_amount/(1024*1024*4);
		} else {
			cmon->memory_size = 16;
		}
	} else {
		cmon->memory_size = cmon->config.ram_size;
	}

	co_debug("configured to %d MB\n", cmon->memory_size);

	if (cmon->memory_size < 8)
		cmon->memory_size = 8;
	else if (cmon->memory_size > 512)
		cmon->memory_size = 512;

	co_debug("after adjustments: %d MB\n", cmon->memory_size);

	cmon->memory_size <<= 20; /* Megify */

	cmon->physical_frames = cmon->memory_size >> CO_ARCH_PAGE_SHIFT;
	cmon->end_physical = CO_ARCH_KERNEL_OFFSET + cmon->memory_size;
	cmon->passage_page_vaddr = CO_VPTR_PASSAGE_PAGE;

	rc = alloc_pseudo_physical_memory(cmon);
        if (!CO_OK(rc)) {
		goto out_free_os_dep;
	}

        rc = co_os_timer_create(&timer_callback, cmon, 10, &cmon->timer);
        if (!CO_OK(rc)) {
                co_debug("creating host OS timer (%d)\n", rc);
                goto out_free_pp;
        }

	rc = co_monitor_load_configuration(cmon);
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

co_rc_t co_monitor_load_section(co_monitor_t *cmon, co_monitor_ioctl_load_section_t *params)
{
	co_rc_t rc = CO_RC(OK);

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED)
		return CO_RC(ERROR);

	if (params->user_ptr) {
		co_debug("loading section at 0x%x (0x%x bytes)\n", params->address, params->size);
		rc = co_monitor_copy_and_create_pfns(
			cmon, params->address, params->size, params->buf);
	}

	return rc;
}

co_rc_t co_monitor_load_initrd(co_monitor_t *cmon, co_monitor_ioctl_load_initrd_t *params)
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

	rc = co_monitor_copy_and_create_pfns(cmon, address, params->size, params->buf);

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

co_rc_t co_monitor_start(co_monitor_t *cmon)
{
	co_rc_t rc;

	if (cmon->state != CO_MONITOR_STATE_INITIALIZED) {
		co_debug("invalid state\n");
		return CO_RC(ERROR);
	}
		
	rc = colinux_init(cmon);
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

co_rc_t co_monitor_run(co_monitor_t *cmon,
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
			ret = co_monitor_iteration(cmon);
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
		return co_monitor_load_section(cmon, (co_monitor_ioctl_load_section_t *)io_buffer);
	}
	case CO_MONITOR_IOCTL_LOAD_INITRD: {
		return co_monitor_load_initrd(cmon, (co_monitor_ioctl_load_initrd_t *)io_buffer);
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

