/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_KERNEL_FILE_SYSTEM_H__
#define __COLINUX_KERNEL_FILE_SYSTEM_H__

#include <colinux/common/list.h>
#include <colinux/common/common.h>
#include <colinux/common/config.h>

typedef struct  co_filesystem_name {
	co_list_t node;
	unsigned char type;
	char name[1];
} co_filesystem_name_t;

typedef struct co_filesystem_dir_names {
	co_list_t list;
	int refcount;
} co_filesystem_dir_names_t;

typedef struct co_inode {
	co_list_t flat_node;
	co_list_t node;
	struct co_inode *parent;
	co_list_t sub_inodes;
	char *name;
	co_filesystem_dir_names_t *names;
} co_inode_t;

typedef struct co_filesystem {
	co_list_t list_inodes;
	co_cofsdev_desc_t *desc;
	co_inode_t *root;
	int inodes_count;
} co_filesystem_t;

struct co_monitor;
extern void co_monitor_file_system(struct co_monitor *cmon, unsigned long unit, 
				   enum fuse_opcode opcode, unsigned long *params);

extern void co_filesystem_getdir_free(co_filesystem_dir_names_t *names);

extern co_rc_t co_monitor_file_system_init(struct co_monitor *cmon, unsigned long unit, 
					   co_cofsdev_desc_t *desc);

extern void co_monitor_unregister_filesystems(struct co_monitor *cmon);

#endif
