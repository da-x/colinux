/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <asm/page.h>
#include <asm/pgtable.h>

#include <memory.h>

#include <colinux/kernel/transfer.h>

/*
 * This code allows direct copying from and to the Linux kernel address space.
 *
 * It's main function is to split a poriton of the psuedo physical address space
 * to single pages that can be read from host OS side. It does so using the page
 * tables that map the pseudo physical RAM into Linux's address space.
 */

co_rc_t co_monitor_host_colx_transfer(
	co_monitor_t *cmon, 
	void *host_data, 
	co_monitor_transfer_func_t host_func, 
	vm_ptr_t colx, 
	unsigned long size, 
	co_monitor_transfer_dir_t dir
	)
{
	unsigned long pfn;
	unsigned char *page;
	unsigned long one_copy;
	co_rc_t rc;

	if ((colx < __PAGE_OFFSET) || (colx >= cmon->end_physical)) {
		co_debug("Error, transfer off bounds: %x\n", colx);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}

	if ((colx + size < __PAGE_OFFSET) || (colx + size > cmon->end_physical)) {
		co_debug("Error, transfer end off bounds: %x\n", colx + size);
		return CO_RC(TRANSFER_OFF_BOUNDS);
	}	

	while (size > 0) {
		pfn = pte_val(cmon->page_tables[CO_PFN(colx)]) >> PAGE_SHIFT;
		if (pfn == 0) {
			co_debug("Error, transfer from invalid page at vm addr: %x\n", colx);
			return CO_RC(TRANSFER_OFF_BOUNDS);
		}

		page = cmon->pa_to_host_va[pfn];
		if (page == NULL) {
			co_debug("Error, transfer from invalid page at vm addr: %x\n", colx);
			return CO_RC(TRANSFER_OFF_BOUNDS);
		}
		
		one_copy = ((colx + PAGE_SIZE) & PAGE_MASK) - colx;
		if (one_copy > size)
			one_copy = size;

		rc = host_func(cmon, host_data, page + (colx & ~PAGE_MASK), one_copy, dir);
		if (!CO_OK(rc))
			return rc;

		size -= one_copy;	
		colx += one_copy;
	}

	return CO_RC(OK);
} 


static co_rc_t co_monitor_transfer_memcpy(co_monitor_t *cmon, void *host_data, void *colx, 
					  unsigned long size, co_monitor_transfer_dir_t dir)
{
	unsigned char **host = (unsigned char **)host_data;

	if (dir == CO_MONITOR_TRANSFER_FROM_HOST)
		memcpy(colx, *host, size);
	else
		memcpy(*host, colx, size);

	(*host) += size;

	return CO_RC(OK);
}

static co_rc_t co_monitor_host_colx_copy(co_monitor_t *cmon, void *host, vm_ptr_t colx, 
					 unsigned long size, co_monitor_transfer_dir_t dir)
{
	return co_monitor_host_colx_transfer(cmon, &host, co_monitor_transfer_memcpy,
					     colx, size, dir);
}


co_rc_t co_monitor_host_to_colx(co_monitor_t *cmon, void *from, 
				vm_ptr_t to, unsigned long size)
{
	return co_monitor_host_colx_copy(cmon, from, to, size, CO_MONITOR_TRANSFER_FROM_HOST);
}


co_rc_t co_monitor_colx_to_host(co_monitor_t *cmon, vm_ptr_t from, 
				void *to, unsigned long size)
{
	return co_monitor_host_colx_copy(cmon, to, from, size, CO_MONITOR_TRANSFER_FROM_COLX);
}
