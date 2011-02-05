/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */


#include "monitor.h"
#include "pages.h"

#include <colinux/common/libc.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/manager.h>
#include <colinux/kernel/pages.h>
#include <colinux/arch/mmu.h>

co_rc_t co_monitor_host_linuxvm_transfer_map(
	co_monitor_t*	cmon,
	vm_ptr_t	vaddr,
	unsigned long 	size,
	unsigned char** start,
	unsigned char** page,
	co_pfn_t*	ppfn
) {
	unsigned long one_copy;
	co_rc_t rc;

	if ((vaddr < CO_ARCH_KERNEL_OFFSET) || (vaddr >= cmon->end_physical)) {
		co_debug_error("monitor: transfer map: vaddr off bounds: %p", (void*)vaddr);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	if ((vaddr + size < CO_ARCH_KERNEL_OFFSET) || (vaddr + size > cmon->end_physical)) {
		co_debug_error("monitor: transfer map: end off bounds: %p", (void*)(vaddr + size));
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	one_copy = ((vaddr + CO_ARCH_PAGE_SIZE) & CO_ARCH_PAGE_MASK) - vaddr;
	if (size <= 0 || size > one_copy) {
		co_debug_error("monitor: transfer map: bad size: %ld (%ld)", size, one_copy);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	rc = co_monitor_get_pfn(cmon, vaddr, ppfn);
	if (!CO_OK(rc))
		return rc;

	*page = co_os_map(cmon->manager, *ppfn);
	*start = *page + (vaddr & ~CO_ARCH_PAGE_MASK);

	return CO_RC(OK);
}

/*
 * This code allows direct copying from and to the Linux kernel address space.
 *
 * It's main function is to split a poriton of the psuedo physical address space
 * to single pages that can be read from host OS side. It does so using the page
 * tables that map the pseudo physical RAM into Linux's address space.
 */

co_rc_t co_monitor_host_linuxvm_transfer(
	co_monitor_t *cmon,
	void *host_data,
	co_monitor_transfer_func_t host_func,
	vm_ptr_t vaddr,
	unsigned long size,
	co_monitor_transfer_dir_t dir
	)
{
	co_pfn_t pfn;
	unsigned char *page;
	unsigned long one_copy;
	co_rc_t rc;

	if ((vaddr < CO_ARCH_KERNEL_OFFSET) || (vaddr >= cmon->end_physical)) {
		co_debug_error("monitor: transfer: vaddr off bounds: %p", (void*)vaddr);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	if ((vaddr + size < CO_ARCH_KERNEL_OFFSET) || (vaddr + size > cmon->end_physical)) {
		co_debug_error("monitor: transfer: end off bounds: %p", (void*)(vaddr + size));
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	while (size > 0) {
		rc = co_monitor_get_pfn(cmon, vaddr, &pfn);
		if (!CO_OK(rc))
			return rc;

		one_copy = ((vaddr + CO_ARCH_PAGE_SIZE) & CO_ARCH_PAGE_MASK) - vaddr;
		if (one_copy > size)
			one_copy = size;

		page = co_os_map(cmon->manager, pfn);
		rc = host_func(cmon, host_data, page + (vaddr & ~CO_ARCH_PAGE_MASK), one_copy, dir);
		co_os_unmap(cmon->manager, page, pfn);

		if (!CO_OK(rc))
			return rc;

		size -= one_copy;
		vaddr += one_copy;
	}

	return CO_RC(OK);
}


static co_rc_t co_monitor_transfer_memcpy(co_monitor_t *cmon, void *host_data, void *linuxvm,
					  unsigned long size, co_monitor_transfer_dir_t dir)
{
	unsigned char **host = (unsigned char **)host_data;

	if (dir == CO_MONITOR_TRANSFER_FROM_HOST)
		co_memcpy(linuxvm, *host, size);
	else
		co_memcpy(*host, linuxvm, size);

	(*host) += size;

	return CO_RC(OK);
}

static co_rc_t co_monitor_host_linuxvm_copy(co_monitor_t *cmon, void *host, vm_ptr_t linuxvm,
					    unsigned long size, co_monitor_transfer_dir_t dir)
{
	return co_monitor_host_linuxvm_transfer(cmon, &host, co_monitor_transfer_memcpy,
						linuxvm, size, dir);
}


co_rc_t co_monitor_host_to_linuxvm(co_monitor_t *cmon, void *from,
				   vm_ptr_t to, unsigned long size)
{
	return co_monitor_host_linuxvm_copy(cmon, from, to, size, CO_MONITOR_TRANSFER_FROM_HOST);
}


co_rc_t co_monitor_linuxvm_to_host(co_monitor_t *cmon, vm_ptr_t from,
				   void *to, unsigned long size)
{
	return co_monitor_host_linuxvm_copy(cmon, to, from, size, CO_MONITOR_TRANSFER_FROM_LINUX);
}

