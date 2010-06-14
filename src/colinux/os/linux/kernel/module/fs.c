#include <colinux/os/kernel/filesystem.h>

co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir,
				      char **out_name, int add)
{
	/* TODO */
	return CO_RC(ERROR);
}

int co_os_fs_add_last_component(co_pathname_t *dirname)
{
	/* TODO */
	return 0;
}

co_rc_t co_os_fs_dir_inode_to_path(co_filesystem_t *fs, co_inode_t *dir,
				   char **out_name, char *name)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_read_write(struct co_monitor *linuxvm, char *filename,
			      unsigned long long offset, unsigned long size,
			      vm_ptr_t src_buffer, bool_t read)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_set_attr(char *filename, unsigned long valid, struct fuse_attr *attr)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_get_attr(char *filename, struct fuse_attr *attr)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_unlink(char *filename)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_mkdir(char *dirname)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_rename(char *filename, char *dest_filename)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_mknod(co_filesystem_t *filesystem, char *filename, unsigned long mode)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_getdir(char *dirname, co_filesystem_dir_names_t *names)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_fs_stat(co_filesystem_t *filesystem, struct fuse_statfs_out *statfs)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_file_rmdir(char *filename)
{
	/* TODO */
	return CO_RC(ERROR);
}

co_rc_t co_os_fs_dir_join_unix_path(co_pathname_t *dirname, const char *addition)
{
	/* TODO */
	return CO_RC(ERROR);
}
