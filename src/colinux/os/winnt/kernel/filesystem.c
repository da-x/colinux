#include <colinux/common/libc.h>
#include <colinux/kernel/filesystem.h>
#include <colinux/os/alloc.h>
#include <colinux/os/kernel/filesystem.h>

co_rc_t co_os_fs_inode_to_path(co_filesystem_t *fs, co_inode_t *dir, char **out_name, int add)
{
	co_inode_t *dir_scan = dir;
	int len;
	int depth = 0;
	const int max_depth = 256;
	char *names[max_depth];
	char *fullname, *adding;
	int namelen = add;

	while (dir_scan  &&  dir_scan->name  &&  depth < max_depth) {
		names[depth++] = dir_scan->name;
		namelen += co_strlen(dir_scan->name) + 1;
		dir_scan = dir_scan->parent;
	}

	len = co_strlen(fs->base_path);
	fullname = co_os_malloc(len + namelen + 2);
	if (!fullname)
		return CO_RC(OUT_OF_MEMORY);

	co_memcpy(fullname, fs->base_path, len);
	adding = fullname + len;

	while (depth-- > 0) {
		*adding++ = '\\';
		len = co_strlen(names[depth]);
		co_memcpy(adding, names[depth], len);
		adding += len;
	}

	/* impl. 'co_os_fs_add_last_component' directly here */
	if (add > 0) {
		if (adding > fullname  &&  *(adding-1) != '\\') {
			*adding++ = '\\';
		}
	}

	*adding = '\0';

	*out_name = fullname;
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
				   char **out_name, char *name)
{
	co_rc_t rc;
	int len;

	if (name && *name)
		len = 1+co_strlen(name);
	else
		len = 0;

	rc = co_os_fs_inode_to_path(fs, dir, out_name, len);
	if (!CO_OK(rc))
		return rc;

	if (len)
		co_memcpy(&(*out_name)[co_strlen(*out_name)], name, len);

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
