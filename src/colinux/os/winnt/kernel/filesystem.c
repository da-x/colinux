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

	co_snprintf(start_adding, terminal - start_adding, "%s", &fs->desc->pathname[0]);
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
	if (len > 0  &&  (*dirname)[len] != '\\'  && (len + 2) < sizeof(*dirname)) {
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
