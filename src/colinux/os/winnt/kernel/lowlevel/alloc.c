/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <asm/page.h>

#include "../ddk.h"

#include <colinux/os/alloc.h>
#include <colinux/os/kernel/alloc.h>

#ifdef DEBUG_CO_OS_ALLOC
static int allocs;
#endif

void *co_os_alloc_pages(unsigned long pages)
{
	void *ret;

	if (pages == 0)
		KeBugCheck(0x11117777);

	ret = MmAllocateNonCachedMemory(pages * PAGE_SIZE);

#ifdef DEBUG_CO_OS_ALLOC		
	if (ret) {
		allocs++;
		co_debug("%s:%d(%d)->%x\n", __FUNCTION__, allocs, pages, ret);
	}
#endif

	return ret;
}

void co_os_free_pages(void *ptr, unsigned long pages)
{
	if (ptr == 0)
		KeBugCheck(0x11117777 + 1);

	if (pages == 0)
		KeBugCheck(0x11117777 + 2);

#ifdef DEBUG_CO_OS_ALLOC		
	co_debug("%s:%d(%x,%d)\n", __FUNCTION__,allocs, ptr, pages);
	allocs--;
#endif
	MmFreeNonCachedMemory(ptr, pages * PAGE_SIZE);
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
		co_debug("%s:%d(%d)->%x\n", __FUNCTION__, allocs, bytes, ret);
	}
#endif

	return ret;
}

void co_os_free(void *ptr)
{
#ifdef DEBUG_CO_OS_ALLOC		
	co_debug("%s:%d(%x)\n", __FUNCTION__, allocs, ptr);
	allocs--;
#endif
	if (ptr == 0)
		KeBugCheck(0x11117777 + 4);

	ExFreePoolWithTag(ptr, CO_OS_POOL_TAG);
}
