#include "linux_inc.h"

#include <colinux/user/debug.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/misc.h>
#include <asm/mman.h>

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

co_rc_t co_os_userspace_map(void *address, unsigned long pages, void **user_address_out, void **handle_out)
{
	struct file *filp;
	unsigned long pa;

	filp = filp_open("/dev/kmem", O_RDWR | O_LARGEFILE, 0);
	if (!filp)
		return CO_RC(ERROR);
	
	pa = co_os_virt_to_phys(address);
	if (!pa) {
		filp_close(filp, NULL);
		return CO_RC(ERROR);
	}

	void *result = (void *)do_mmap_pgoff(filp, 0, pages << PAGE_SHIFT, 
					     PROT_EXEC | PROT_READ | PROT_WRITE, 
					     MAP_PRIVATE, pa >> PAGE_SHIFT);
	if (IS_ERR(result)) {
		filp_close(filp, NULL);
		return CO_RC(ERROR);
	}

	*user_address_out = result;
	*handle_out = filp;

	return CO_RC(OK);
}

void co_os_userspace_unmap(void *user_address, void *handle, unsigned long pages)
{
	struct file *filp = (struct file *)handle;

	if (user_address)
		do_munmap(current->mm, (unsigned long)user_address, pages << PAGE_SHIFT);

	filp_close(filp, NULL);
}
