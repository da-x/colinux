/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_LINUX_KERNEL_MANAGER_H__
#define __CO_LINUX_KERNEL_MANAGER_H__

#include <linux/proc_fs.h>
#include <colinux/os/kernel/manager.h>
#include <colinux/os/kernel/alloc.h>

struct co_osdep_manager {
	struct proc_dir_entry *proc_root;
	struct proc_dir_entry *proc_ioctl;
};

extern co_manager_t *global_manager;

#endif
