#include "linux_inc.h"

#include <colinux/user/debug.h>
#include <colinux/os/alloc.h>

static int blocks = 0;

void *co_os_malloc(unsigned long bytes)
{
	void *ret;

	blocks++;

	ret = kmalloc(bytes, GFP_KERNEL);

#if (0)
	co_debug_lvl(allocations, 11, "BLOCK ALLOC %d: %x %d\n", blocks-1, ret, bytes);
#endif

	return ret;
}

void co_os_free(void *ptr)
{
	blocks--;
	kfree(ptr);

#if (0)
	co_debug_lvl(allocations, 11, "BLOCK FREE %d: %x\n", blocks, ptr);
#endif
}

