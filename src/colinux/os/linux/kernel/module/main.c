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

/* Prototype must be local, checks the linker here */
extern int regparm_check(int p1, int p2, int p3);

static int __init colinux_module_init(void)
{
	co_rc_t rc;

	printk(KERN_INFO "colinux: loaded version " COLINUX_VERSION " (compiled on " __DATE__ " " __TIME__ ")\n");

	/* Function params must check, before any other coLinux function from prelinked_driver will call */
	if (regparm_check(1, 2, 3)) {
		printk(KERN_ERR "colinux: Don't run on regparm enabled host kernels kernels\n");
		return -EILSEQ;
	}

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

	printk(KERN_INFO "colinux: module unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(colinux_module_init);
module_exit(colinux_module_exit);
