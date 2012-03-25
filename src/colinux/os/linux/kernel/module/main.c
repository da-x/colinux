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
#include <linux/fs.h>
#include <linux/debugfs.h>

struct dentry  *pgentry;

/* 
 */
int mymmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start;

    /* check length - only handle 1 page case */
    if (length > PAGE_SIZE) {
        return -EIO;
    }

    printk("len 0x%lx pg 0x%lx", length, PAGE_SIZE);   
    /* map the whole physically contiguous area in one piece */
    if ((ret = remap_pfn_range(vma,
                               vma->vm_start,
                               virt_to_phys((void *)(vma->vm_pgoff<<PAGE_SHIFT)) >> PAGE_SHIFT,
                               length,
                               vma->vm_page_prot)) < 0) {
        return ret;
    }   
    return 0;
}

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
        printk("mmap vm_pgoff 0x%lx \n", vma->vm_pgoff);
        return mymmap(filp, vma);
}

int my_close(struct inode *inode, struct file *filp)
{
    return 0;
}

int my_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations my_fops = {
    .open = my_open,
    .release = my_close,
    .mmap = my_mmap,
};

static int __init colinux_module_init(void)
{
	co_rc_t rc;

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

    pgentry = debugfs_create_file("commap", 0644, NULL, NULL, &my_fops);
    return 0;
}


static void __exit colinux_module_exit(void)
{
	if (co_global_manager != NULL) {
		co_manager_t *manager = co_global_manager;
		co_manager_unload(manager);
		co_global_manager = NULL;
		co_os_free(manager);
	}

    debugfs_remove(pgentry);
}

MODULE_LICENSE("GPL");
module_init(colinux_module_init);
module_exit(colinux_module_exit);
