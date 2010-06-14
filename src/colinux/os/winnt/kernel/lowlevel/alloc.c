/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

/* WinNT host: allocate pages in WinNT kernel space for
 * CPL0 driver 'linux.sys'.
 */

#include "../ddk.h"

#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>

#ifdef DEBUG_CO_OS_ALLOC
static int allocs;
static int alloc_reenter;

#define co_debug_allocations(fmt, ...) do { \
        if (!alloc_reenter) { \
		alloc_reenter++; \
		co_debug_lvl(allocations, 11, fmt, ## __VA_ARGS__); \
		alloc_reenter--; \
	} \
} while (0)
#endif

void *co_os_alloc_pages(unsigned int pages)
{
	void *ret;

	if (pages == 0)
		KeBugCheck(0x11117777);

	ret = MmAllocateNonCachedMemory(pages * CO_ARCH_PAGE_SIZE);

#ifdef DEBUG_CO_OS_ALLOC
	if (ret) {
		allocs++;
		co_debug_allocations("PAGE ALLOC %d(%u) - %p", allocs, pages, ret);
	}
#endif

	return ret;
}

void co_os_free_pages(void *ptr, unsigned int pages)
{
	if (ptr == 0)
		KeBugCheck(0x11117777 + 1);

	if (pages == 0)
		KeBugCheck(0x11117777 + 2);

#ifdef DEBUG_CO_OS_ALLOC
	co_debug_allocations("PAGE FREE %d(%u) - %p", allocs, pages, ptr);
	allocs--;
#endif
	MmFreeNonCachedMemory(ptr, pages * CO_ARCH_PAGE_SIZE);
}

#define CO_OS_POOL_TAG (('c' << 0) | ('o' << 8) |  ('l' << 16) | ('x' << 24))

void *co_os_malloc(unsigned long bytes)
{
	void *ret;

	if (bytes == 0)
		KeBugCheck(0x11117777 + 3);

	ret = ExAllocatePoolWithTag(NonPagedPool, bytes, CO_OS_POOL_TAG);


#ifdef DEBUG_CO_OS_ALLOC
	if (ret) {
		allocs++;
		co_debug_allocations("MEM ALLOC %d(%lu) - %p", allocs, bytes, ret);
	}
#endif

	return ret;
}

void co_os_free(void *ptr)
{
#ifdef DEBUG_CO_OS_ALLOC
	co_debug_allocations("MEM FREE %d - %p", allocs, ptr);
	allocs--;
#endif
	if (ptr == 0)
		KeBugCheck(0x11117777 + 4);

	ExFreePoolWithTag(ptr, CO_OS_POOL_TAG);
}

co_rc_t co_os_userspace_map(void *address, unsigned int pages, void **user_address_out, void **handle_out)
{
	void *user_address;
	unsigned long memory_size = ((unsigned long)pages) << CO_ARCH_PAGE_SHIFT;
	PMDL mdl;

	mdl = IoAllocateMdl(address, memory_size, FALSE, FALSE, NULL);
	if (!mdl)
		return CO_RC(ERROR);

	MmBuildMdlForNonPagedPool(mdl);
	user_address = MmMapLockedPagesSpecifyCache(mdl, UserMode, MmCached, NULL, FALSE, HighPagePriority);
	if (!user_address) {
		IoFreeMdl(mdl);
		return CO_RC(ERROR);
	}

	*handle_out = (void *)mdl;
	*user_address_out = PAGE_ALIGN(user_address) + MmGetMdlByteOffset(mdl);

	return CO_RC(OK);
}

void co_os_userspace_unmap(void *user_address, void *handle, unsigned int pages)
{
	PMDL mdl = (PMDL)handle;

	if (user_address)
		MmUnmapLockedPages(user_address, mdl);

	IoFreeMdl(mdl);
}
