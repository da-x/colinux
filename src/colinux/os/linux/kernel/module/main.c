/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include "linux_inc.h"
#include <colinux/common/version.h>
#include "manager.h"
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <asm/io.h>

/* character device structures */
static dev_t mmap_dev;
static struct cdev mmap_cdev;

/* methods of the character device */
static int mmap_open(struct inode *inode, struct file *filp);
static int mmap_release(struct inode *inode, struct file *filp);
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma);
/* the file operations, i.e. all character device methods */
static struct file_operations mmap_fops = {
        .open = mmap_open,
        .release = mmap_release,
        .mmap = mmap_mmap,
        .owner = THIS_MODULE,
};

// internal data
// length of the two memory areas
#define NPAGES 16
// pointer to the vmalloc'd area - alway page aligned
static int *vmalloc_area;
// pointer to the kmalloc'd area, rounded up to a page boundary
static int *kmalloc_area;
// original pointer for kmalloc'd area as returned by kmalloc
static void *kmalloc_ptr;

/* character device open method */
static int mmap_open(struct inode *inode, struct file *filp)
{
        return 0;
}
/* character device last close method */
static int mmap_release(struct inode *inode, struct file *filp)
{
        return 0;
}

// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem(struct file *filp, struct vm_area_struct *vma)
{
        int ret;
        long length = vma->vm_end - vma->vm_start;

        /* check length - do not allow larger mappings than the number of
           pages allocated */
        if (length > NPAGES * PAGE_SIZE)
                return -EIO;

        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma,
                                   vma->vm_start,
                                   virt_to_phys((void *)kmalloc_area) >> PAGE_SHIFT,
                                   length,
                                   vma->vm_page_prot)) < 0) {
                return ret;
        }
        
        return 0;
}
/* character device mmap method */
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return mmap_kmem(filp, vma);
}


static int __init colinux_module_init(void)
{
	co_rc_t rc;
    int ret = 0;
	printk(KERN_INFO "colinux: loaded version " COLINUX_VERSION " (compiled on " __DATE__ " " __TIME__ ")\n");

	co_global_manager = co_os_malloc(sizeof(co_manager_t));
	if (co_global_manager == NULL) {
		printk(KERN_ERR "colinux: allocation error\n");
		return -ENOMEM;
	}

	rc = co_manager_load(co_global_manager);
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_PAE_ENABLED) {
			printk("colinux: PAE is enabled, cannot continue\n");
			return -ENOSYS;
		}
		printk(KERN_ERR "colinux: manager load failure: %x\n", (int)rc);
		return -ENXIO;
	}

    /* get the major number of the character device */
    if ((ret = alloc_chrdev_region(&mmap_dev, 0, 1, "commap")) < 0) {
        printk(KERN_ERR "could not allocate major number for mmap\n");
        goto out_vfree;
    }

    /* initialize the device structure and register the device with the kernel */
    cdev_init(&mmap_cdev, &mmap_fops);
    if ((ret = cdev_add(&mmap_cdev, mmap_dev, 1)) < 0) {
        printk(KERN_ERR "could not allocate chrdev for mmap\n");
        goto out_unalloc_region;
    }

    return 0;

out_unalloc_region:
    unregister_chrdev_region(mmap_dev, 1);
out_vfree:
    //vfree(vmalloc_area);
out_kfree:
    //kfree(kmalloc_ptr);
out:
    return ret;
}


static void __exit colinux_module_exit(void)
{
	if (co_global_manager != NULL) {
		co_manager_t *manager = co_global_manager;
		co_manager_unload(manager);
		co_global_manager = NULL;
		co_os_free(manager);
	}
        /* remove the character deivce */
        cdev_del(&mmap_cdev);
        unregister_chrdev_region(mmap_dev, 1);
}

MODULE_LICENSE("GPL");
module_init(colinux_module_init);
module_exit(colinux_module_exit);
