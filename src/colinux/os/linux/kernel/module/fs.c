#include <colinux/os/kernel/filesystem.h>

co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, co_pathname_t *out_name)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_getdir(co_filesystem_t *fs, co_inode_t *dir, co_filesystem_dir_names_t *names)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_getattr(co_filesystem_t *fs, co_inode_t *dir,  char *name, struct fuse_attr *attr)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_read_write(struct co_monitor *linuxvm, co_filesystem_t *filesystem, 
				  co_inode_t *inode, unsigned long long offset, unsigned long size,
				  vm_ptr_t src_buffer, bool_t read)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_mknod(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
			     unsigned long rdev, char *name, int *ino, struct fuse_attr *attr)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_mkdir(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
			     char *name)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_unlink(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_rmdir(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_rename(co_filesystem_t *filesystem, co_inode_t *dir, co_inode_t *new_dir,
			      char *oldname, char *newname)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_inode_set_attr(co_filesystem_t *filesystem, co_inode_t *inode,
				unsigned long valid, struct fuse_attr *attr)
{
	/* TODO */
	return CO_RC(ERROR);
}

