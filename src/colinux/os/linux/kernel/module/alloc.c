
/* Linux host: allocate memory in linux kernel space */

#include "linux_inc.h"

#include <colinux/user/debug.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/misc.h>
#include <asm/mman.h>

#ifdef DEBUG_CO_KMALLOC
static int blocks = 0;
#endif

void *co_os_malloc(unsigned long bytes)
{
	void *ret;

	ret = kmalloc(bytes, GFP_KERNEL);

#ifdef DEBUG_CO_KMALLOC
	co_debug_lvl(allocations, 11, "BLOCK ALLOC %d: %x %d", blocks++, ret, bytes);
#endif

	return ret;
}

void co_os_free(void *ptr)
{
	kfree(ptr);

#ifdef DEBUG_CO_KMALLOC
	co_debug_lvl(allocations, 11, "BLOCK FREE %d: %x", --blocks, ptr);
#endif
}

co_rc_t co_os_userspace_map(void *address, unsigned int pages, void **user_address_out, void **handle_out)
{
	struct file *filp;
	unsigned long pa;
	void *result;

	pa = co_os_virt_to_phys(address);
	if (!pa) {
		co_debug_error("co_os_virt_to_phys failed");
		//filp_close(filp, NULL);
		return CO_RC(ERROR);
	}
	printk("kern vmaddr 0x%x, phyaddr 0x%x\n", address, pa);

	//filp = filp_open("/dev/kmem", O_RDWR | O_LARGEFILE, 0);
	filp = filp_open("/sys/kernel/debug/commap", O_RDWR | O_LARGEFILE, 0);
	if (IS_ERR(filp) || NULL==filp) {
		co_debug_error("open commap failed %d", filp);
		return CO_RC(ERROR);
	} else {
		result = (void *)do_mmap_pgoff(filp, 0, ((unsigned long)pages) << PAGE_SHIFT,
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
			co_debug_error("do_mmap_pgoff failed (errno %ld)", PTR_ERR(result));
			filp_close(filp, NULL);
			return CO_RC(ERROR);
		}
	}
	*user_address_out = result;
	*handle_out = filp;

	return CO_RC(OK);
}

void co_os_userspace_unmap(void *user_address, void *handle, unsigned int pages)
{
	struct file *filp;
	if (IS_ERR(handle) || NULL==handle) {
		co_debug("TODO, unmap userspace?");
	} else {
		filp = (struct file *)handle;

		if (user_address)
			do_munmap(current->mm, (unsigned long)user_address, ((unsigned long)pages) << PAGE_SHIFT);

		filp_close(filp, NULL);

	}
}
