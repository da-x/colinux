/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <asm/page.h>
#include <asm/pgtable.h>

#include <memory.h>

#include <colinux/os/kernel/alloc.h>
#include <colinux/kernel/transfer.h>
#include <colinux/kernel/manager.h>

co_rc_t co_monitor_get_pfn(co_monitor_t *cmon, vm_ptr_t address, co_pfn_t *pfn)
{
	unsigned long current_pfn, pfn_group, pfn_index;

	current_pfn = (address >> PAGE_SHIFT);
	pfn_group = current_pfn / PTRS_PER_PTE;
	pfn_index = current_pfn % PTRS_PER_PTE;

	if (cmon->pp_pfns[pfn_group] == NULL)
		return CO_RC(ERROR);

	*pfn = cmon->pp_pfns[pfn_group][pfn_index];

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

	if ((vaddr < __PAGE_OFFSET) || (vaddr >= cmon->end_physical)) {
		co_debug("monitor: transfer: off bounds: %x\n", vaddr);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	if ((vaddr + size < __PAGE_OFFSET) || (vaddr + size > cmon->end_physical)) {
		co_debug("monitor: transfer: end off bounds: %x\n", vaddr + size);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}	

	while (size > 0) {
		rc = co_monitor_get_pfn(cmon, vaddr, &pfn);
		if (!CO_OK(rc))
			return rc;		
		
		one_copy = ((vaddr + PAGE_SIZE) & PAGE_MASK) - vaddr;
		if (one_copy > size)
			one_copy = size;

		page = (char *)co_os_map(cmon->manager, pfn);
		rc = host_func(cmon, host_data, page + (vaddr & ~PAGE_MASK), one_copy, dir);
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
		memcpy(linuxvm, *host, size);
	else
		memcpy(*host, linuxvm, size);

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

typedef co_rc_t (*co_split_by_pages_callback_t)(
	unsigned long offset,
	unsigned long size,
	void **data
	);

co_rc_t co_split_by_pages_and_callback(
	unsigned long offset,
	unsigned long size,
	void **data,
	co_split_by_pages_callback_t func
	)
{
	unsigned long one_copy;
	co_rc_t rc;

	/* Split page one by one (including partials) */
	while (size > 0) {
		one_copy = ((offset + PAGE_SIZE) & PAGE_MASK) - offset;
		if (one_copy > size)
			one_copy = size;

		rc = func(offset, one_copy, data); 
		if (!CO_OK(rc))
			return rc;

		size -= one_copy;
		offset += one_copy;
	}

	return CO_RC(OK);
}

typedef co_rc_t (*co_manager_create_pfns_callback_t)(
	void *mapped_ptr,
	void **data,
	unsigned long size
	);

typedef struct {
	co_manager_create_pfns_callback_t func;
	void **data;
	co_monitor_t *monitor;
} co_manager_create_pfns_callback_data_t;

co_rc_t co_manager_create_pfns_and_callback_callback(
	unsigned long offset,
	unsigned long size,
	void **data
	)
{
	co_manager_create_pfns_callback_data_t *cbdata;
	unsigned long current_pfn, pfn_group;
	void *mapped_page;
	co_pfn_t real_pfn, **pp_pfns;
	co_rc_t rc;

	cbdata = (typeof(cbdata))(data);
	pp_pfns = cbdata->monitor->pp_pfns;

	current_pfn = (offset >> PAGE_SHIFT);
	pfn_group = current_pfn / PTRS_PER_PTE;
	current_pfn = current_pfn % PTRS_PER_PTE;

	if (pp_pfns[pfn_group] == NULL) {
		rc = co_monitor_malloc(cbdata->monitor, sizeof(co_pfn_t)*PTRS_PER_PTE, (void **)&pp_pfns[pfn_group]);
		if (!pp_pfns[pfn_group])
			return CO_RC(ERROR);

		memset(pp_pfns[pfn_group], 0, sizeof(co_pfn_t)*PTRS_PER_PTE);
	}
		
	real_pfn = pp_pfns[pfn_group][current_pfn];
	if (real_pfn == 0) {
		rc = co_manager_get_page(cbdata->monitor->manager, &real_pfn);
		if (!CO_OK(rc))
			return rc;

		pp_pfns[pfn_group][current_pfn] = real_pfn;
	}
	
	mapped_page = co_os_map(cbdata->monitor->manager, real_pfn);
	rc = cbdata->func(mapped_page + (offset & (~PAGE_MASK)), cbdata->data, size);
	co_os_unmap(cbdata->monitor->manager, mapped_page, real_pfn);

	return CO_RC(OK);
}

co_rc_t co_manager_create_pfns_and_callback(
	co_monitor_t *monitor, 
	vm_ptr_t address,
	unsigned long size,
	co_manager_create_pfns_callback_t func,
	void **data
	)
{
	co_manager_create_pfns_callback_data_t cbdata;

	cbdata.func = func;
	cbdata.data = data;
	cbdata.monitor = monitor;

	return co_split_by_pages_and_callback(address, size, (void **)&cbdata, 
					      co_manager_create_pfns_and_callback_callback);
}

co_rc_t 
co_manager_create_pfns_copy_callback(
	void *mapped_ptr,
	void **data,
	unsigned long size
	)
{
	memcpy(mapped_ptr, *data, size);
	((char *)(*data)) += size;
	return CO_RC(OK);
}

co_rc_t 
co_monitor_copy_and_create_pfns(
	co_monitor_t *monitor, 
	vm_ptr_t address,
	unsigned long size,
	char *source
	)
{
	return co_manager_create_pfns_and_callback(
		monitor, address, size, 
		co_manager_create_pfns_copy_callback,
		(void **)&source);
}

co_rc_t 
co_manager_create_pfns_scan_callback(
	void *mapped_ptr,
	void **data,
	unsigned long size
	)
{
	return CO_RC(OK);
}

co_rc_t 
co_monitor_scan_and_create_pfns(
	co_monitor_t *monitor, 
	vm_ptr_t address,
	unsigned long size
	)
{
	return co_manager_create_pfns_and_callback(
		monitor, address, size, 
		co_manager_create_pfns_scan_callback,
		NULL);
}

co_rc_t 
co_manager_create_pfns_create_ptes_callback(
	void *mapped_ptr,
	void **data,
	unsigned long size
	)
{
	co_pfn_t *pfns = (co_pfn_t *)(*data);
	linux_pte_t *ptes = (linux_pte_t *)mapped_ptr;
	int i;

	for (i=0; i < size/sizeof(linux_pte_t); i++) {
		if (*pfns != 0)
			*ptes = pte_modify(__pte((*pfns) << PAGE_SHIFT), __pgprot(__PAGE_KERNEL));
		else
			*ptes = __pte(0);
		pfns++;
		ptes++;
	}

	((char *)(*data)) = (char *)pfns;

	return CO_RC(OK);
}

co_rc_t
co_monitor_create_ptes(
	co_monitor_t *monitor, 
	vm_ptr_t address,
	unsigned long size,
	co_pfn_t *source
	)
{
	return co_manager_create_pfns_and_callback(
		monitor, address, size, 
		co_manager_create_pfns_create_ptes_callback,
		(void **)&source);
}
