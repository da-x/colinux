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

/*
 * OS-specific pathname helpers.
 */
extern co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir,
				      char **out_name, int need_len);
extern int co_os_fs_add_last_component(co_pathname_t *dirname);
extern co_rc_t co_os_fs_dir_join_unix_path(co_pathname_t *dirname, const char *addition);
extern co_rc_t co_os_fs_dir_inode_to_path(co_filesystem_t *fs, co_inode_t *dir,
					  char **out_name, char *name);

/*
 * OS-specific operations on files.
 */
extern co_rc_t co_os_file_read_write(struct co_monitor *linuxvm, char *filename, unsigned long long offset,
				     unsigned long size, vm_ptr_t src_buffer, bool_t read);
extern co_rc_t co_os_file_set_attr(char *filename, unsigned long valid, struct fuse_attr *attr);
extern co_rc_t co_os_file_get_attr(char *filename, struct fuse_attr *attr);
extern co_rc_t co_os_file_unlink(char *filename);
extern co_rc_t co_os_file_rmdir(char *filename);
extern co_rc_t co_os_file_mkdir(char *dirname);
extern co_rc_t co_os_file_rename(char *filename, char *dest_filename);
extern co_rc_t co_os_file_mknod(co_filesystem_t *filesystem, char *filename, unsigned long mode);
extern co_rc_t co_os_file_getdir(char *dirname, co_filesystem_dir_names_t *names);
extern co_rc_t co_os_file_fs_stat(co_filesystem_t *filesystem, struct fuse_statfs_out *statfs);

#endif
