/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_OS_KERNEL_FILE_SYSTEM_H__
#define __COLINUX_OS_KERNEL_FILE_SYSTEM_H__

#include <colinux/common/common.h>
#include <colinux/kernel/filesystem.h>

extern co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, co_pathname_t *out_name);
extern co_rc_t co_os_fs_getdir(co_filesystem_t *fs, co_inode_t *dir, co_filesystem_dir_names_t *names);
extern co_rc_t co_os_fs_getattr(co_filesystem_t *fs, co_inode_t *dir,  char *name, struct fuse_attr *attr);
extern co_rc_t co_os_fs_inode_read_write(struct co_monitor *linuxvm, co_filesystem_t *filesystem, 
					 co_inode_t *inode, unsigned long long offset, unsigned long size,
					 vm_ptr_t src_buffer, bool_t read);
extern co_rc_t co_os_fs_inode_mknod(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
				    unsigned long rdev, char *name, int *ino, struct fuse_attr *attr);
extern co_rc_t co_os_fs_inode_mkdir(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
				    char *name);
extern co_rc_t co_os_fs_inode_unlink(co_filesystem_t *filesystem, co_inode_t *inode, char *name);
extern co_rc_t co_os_fs_inode_rmdir(co_filesystem_t *filesystem, co_inode_t *inode, char *name);
extern co_rc_t co_os_fs_inode_rename(co_filesystem_t *filesystem, co_inode_t *dir, co_inode_t *new_dir,
				     char *oldname, char *newname);
extern co_rc_t co_os_fs_inode_set_attr(co_filesystem_t *filesystem, co_inode_t *inode,
				       unsigned long valid, struct fuse_attr *attr);

#endif
