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
	void *result;

	filp = filp_open("/dev/kmem", O_RDWR | O_LARGEFILE, 0);
	if (!filp) {
		co_debug("error: co_os_userspace_map: open /dev/kmem failed");
		return CO_RC(ERROR);
	}
	
	pa = co_os_virt_to_phys(address);
	if (!pa) {
		co_debug("error: co_os_userspace_map: co_os_virt_to_phys failed");
		filp_close(filp, NULL);
		return CO_RC(ERROR);
	}

	result = (void *)do_mmap_pgoff(filp, 0, pages << PAGE_SHIFT, 
					     PROT_EXEC | PROT_READ | PROT_WRITE, 
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,12)
					     MAP_SHARED,
#else
					     MAP_PRIVATE,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
					     ((unsigned)__va(pa)) >> PAGE_SHIFT
#else
					     pa >> PAGE_SHIFT
#endif
	);
	if (IS_ERR(result)) {
		co_debug("error: co_os_userspace_map: do_mmap_pgoff failed (errno %ld)", PTR_ERR(result));
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
