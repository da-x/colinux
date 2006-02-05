#include <colinux/common/libc.h>
#include <colinux/kernel/filesystem.h>
#include <colinux/os/kernel/filesystem.h>

co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, co_pathname_t *out_name)
{
	co_inode_t *dir_scan = dir;
	char *terminal = &((*out_name)[sizeof(co_pathname_t)-1]);
	char *end = terminal;
	char *beginning = &(*out_name)[0];
	char *start_adding = beginning;

	co_snprintf(start_adding, terminal - start_adding, "%s", &fs->base_path[0]);
	start_adding += co_strlen(start_adding);

	*terminal = '\0';
	while (dir_scan  &&  dir_scan->name) {
		char *scan_name = dir_scan->name;
		int size = co_strlen(scan_name);
		terminal -= size;
		if (terminal < start_adding) {
			/* path too long */
			return CO_RC(ERROR);
		}
		co_memcpy(terminal, scan_name, size);
		terminal--;
		if (terminal < start_adding) {
			/* path too long */
			return CO_RC(ERROR);
		}
		*terminal = '\\';
		dir_scan = dir_scan->parent;
	}

	co_memmove(start_adding, terminal, end - terminal + 1);

	return CO_RC(OK);
}

int co_os_fs_add_last_component(co_pathname_t *dirname)
{
	int len;

	len = co_strlen(*dirname);
	if (len > 0  &&  (*dirname)[len-1] != '\\'  && (len + 2) < sizeof(*dirname)) {
		(*dirname)[len] = '\\';
		(*dirname)[len + 1] = '\0';
		len++;
	}
	
	return len;
}

co_rc_t co_os_fs_dir_inode_to_path(co_filesystem_t *fs, co_inode_t *dir,
				   co_pathname_t *out_name, char *name)
{
	co_rc_t rc;
	int len;

	rc = co_os_fs_inode_to_path(fs, dir, out_name);
	if (!CO_OK(rc))
		return rc;

	len = co_os_fs_add_last_component(out_name);
	if (name) {
		co_snprintf(&(*out_name)[len], sizeof(*out_name) - len, "%s", name);
	}

	return CO_RC(OK);
}

co_rc_t co_os_fs_dir_join_unix_path(co_pathname_t *dirname, const char *addition)
{
	int len, total_len;
	
	len = co_os_fs_add_last_component(dirname);
	if (*addition == '/')
		addition++;
	
	co_snprintf(&(*dirname)[len], sizeof(*dirname) - len, "%s", addition);
	
	total_len = co_strlen(*dirname);
	
	while (len < total_len) {
		if ((*dirname)[len] == '/') {
			(*dirname)[len] = '\\';
		}
		len++;
	}

	if ((total_len > 0) && (*dirname)[total_len-1] == '\\') {
		(*dirname)[total_len-1] = '\0';
	}
	
	return CO_RC(OK);
}

co_rc_t co_os_fs_get_attr(co_filesystem_t *fs, char *filename, struct fuse_attr *attr)
{
	co_rc_t rc;

	rc = co_os_file_get_attr(filename, attr);
	if (!CO_OK(rc))
		return rc;
	
	if (attr->mode & FUSE_S_IFDIR)
		attr->mode = fs->dir_mode;
	else
		attr->mode = fs->file_mode;

	attr->uid = fs->uid;
	attr->gid = fs->gid;

	return rc;
}
