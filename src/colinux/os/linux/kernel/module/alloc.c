#include "linux_inc.h"

#include <colinux/os/alloc.h>

void *co_os_malloc(unsigned long bytes)
{
	void *ret;

	ret = kmalloc(bytes, GFP_KERNEL);

	return ret;
}

void co_os_free(void *ptr)
{
	kfree(ptr);
}
