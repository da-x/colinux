#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/os/kernel/filesystem.h>
#include <colinux/os/kernel/time.h>
#include <asm/errno.h>

#include "filesystem.h"
#include "monitor.h"
#include "transfer.h"

static co_inode_t *ino_num_to_inode(int ino, co_filesystem_t *filesystem) 
{
	co_inode_t *inode = NULL;

	if (ino == 1)
		return filesystem->root;

        co_list_each_entry(inode, &filesystem->list_inodes, flat_node) {
		if (inode == (co_inode_t *)(ino))
			goto out;
	}

	inode = NULL;

out:
	if (inode == NULL)
		co_debug_lvl(filesystem, 10, "inode %d to inode struct %x\n", ino, inode);
	return inode;
}

static co_inode_t *alloc_inode(co_filesystem_t *filesystem, co_inode_t *parent, const char *name) 
{
	co_inode_t *inode;

	inode = co_os_malloc(sizeof(*inode));
	if (!inode)
		return NULL;

	co_memset(inode, 0, sizeof(*inode));

	if (parent) {
		co_list_add_tail(&inode->node, &parent->sub_inodes);
		inode->parent = parent;
	}

	if (name != NULL) {
		int len = co_strlen(name);
		char *dup = co_os_malloc(len + 1);
		if (!dup) {
			co_os_free(inode);
			return NULL;
		}
		co_memcpy(dup, name, len+1);
		inode->name = dup;
	}

	co_list_init(&inode->sub_inodes);
	co_list_add_tail(&inode->flat_node, &filesystem->list_inodes);

	filesystem->inodes_count++;
	co_debug_lvl(filesystem, 10, "inode [%d] allocated %x child '%s' of %x\n", 
		     filesystem->inodes_count, inode, name ? name : "<root>", parent);
	
	return inode;
}

static void reparent_inode(co_inode_t *node, co_inode_t *new_parent)
{
	co_list_del(&node->node);
	co_list_add_head(&node->node, &new_parent->sub_inodes);
	node->parent = new_parent;
} 

static co_inode_t *find_inode(co_filesystem_t *filesystem, co_inode_t *parent, const char *name) 
{
	co_inode_t *inode = NULL;
	
        co_list_each_entry(inode, &parent->sub_inodes, node) {
		if (co_strcmp(inode->name, name) == 0)
			return inode;
	}

	return NULL;
}

static void free_inode(co_filesystem_t *filesystem, co_inode_t *inode) 
{
	co_list_del(&inode->flat_node);
	if (inode->parent)
		co_list_del(&inode->node);
	if (inode->names)
		co_filesystem_getdir_free(inode->names);
	if (inode->name)
		co_os_free(inode->name);
	co_os_free(inode);
	co_debug_lvl(filesystem, 10, "inode [%d] freed %x\n", filesystem->inodes_count, inode);
	filesystem->inodes_count--;
}

static void free_inode_children(co_filesystem_t *filesystem, co_list_t *delete_now, 
				int level, co_list_t *delete_later)
{
	co_inode_t *inode, *inode_new;

	if (level > 5) {
		co_list_each_entry_safe(inode, inode_new, delete_now, node) {
			co_list_del(&inode->node);
			co_list_add_head(&inode->node, delete_later);
			inode->parent = NULL;
		}
	} else {
		co_list_each_entry_safe(inode, inode_new, delete_now, node) {
			free_inode_children(filesystem, &inode->sub_inodes, level + 1, delete_later);
			free_inode(filesystem, inode);
		}
	}
}

static void free_inode_with_children(co_filesystem_t *filesystem, co_inode_t *inode)
{
	co_list_t to_delete, to_delete_next;
	
	co_list_init(&to_delete);
	free_inode_children(filesystem, &inode->sub_inodes, 0, &to_delete);
	free_inode(filesystem, inode);

	while (!co_list_empty(&to_delete)) {
		co_list_init(&to_delete_next);
		free_inode_children(filesystem, &to_delete, 0, &to_delete_next);

		if (co_list_empty(&to_delete_next))
			break;

		co_list_init(&to_delete);
		free_inode_children(filesystem, &to_delete_next, 0, &to_delete);
	}
}

static co_rc_t inode_forget(co_filesystem_t *filesystem, co_inode_t *inode)
{
	if (inode)
		free_inode_with_children(filesystem, inode);

	return CO_RC(OK);
}

static co_rc_t inode_dir_open(co_filesystem_t *filesystem, co_inode_t *inode)
{
	co_rc_t rc;

	if (!inode)
		return CO_RC(ERROR);

	if (inode->names) {
		inode->names->refcount++;
		return CO_RC(OK);
	} 

	inode->names = co_os_malloc(sizeof(co_filesystem_dir_names_t));
	if (!inode->names)
		return CO_RC(OUT_OF_MEMORY);

	inode->names->refcount = 1;
	co_list_init(&inode->names->list);

	rc = co_os_fs_getdir(filesystem, inode, inode->names);

	if (!CO_OK(rc)) {
		co_os_free(inode->names);
		inode->names = NULL;
	}

	return rc;
}

static co_rc_t inode_open(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long flags)
{
	return CO_RC(OK);
}

static co_rc_t inode_read(co_monitor_t *cmon, co_filesystem_t *filesystem, co_inode_t *inode, 
			  unsigned long long offset, unsigned long size,
			  vm_ptr_t dest_buffer)
{
	return co_os_fs_inode_read_write(cmon, filesystem, inode, offset, size, dest_buffer, PTRUE);
}

static co_rc_t inode_write(co_monitor_t *cmon, co_filesystem_t *filesystem, co_inode_t *inode, 
			   unsigned long long offset, unsigned long size,
			   vm_ptr_t src_buffer)
{
	return co_os_fs_inode_read_write(cmon, filesystem, inode, offset, size, src_buffer, PFALSE);
}

static co_rc_t inode_mknod(co_filesystem_t *filesystem, co_inode_t *dir, unsigned long mode, 
			   unsigned long rdev, char *name, int *ino, struct fuse_attr *attr)
{
	co_rc_t rc;

	attr->size = 0;
	attr->mode = FUSE_S_IFREG | FUSE_S_IRWXU | FUSE_S_IRGRP | FUSE_S_IROTH;
	attr->nlink = 1;
	attr->uid = 0;
	attr->gid = 0;
	attr->rdev = 0;
	attr->_dummy = 0;
	attr->blocks = 0;
	attr->atime = co_os_get_time();
	attr->mtime = co_os_get_time();
	attr->ctime = co_os_get_time();
	
	rc = co_os_fs_inode_mknod(filesystem, dir, mode, rdev, name, ino, attr);
	
	if (CO_OK(rc)) {
		co_inode_t *inode = NULL;

		inode = find_inode(filesystem, dir, name);
		if (!inode)
			inode = alloc_inode(filesystem, dir, name);
		*ino = (unsigned long)inode;
	}

	return rc;
}

static co_rc_t inode_mkdir(co_filesystem_t *filesystem, co_inode_t *inode, unsigned long mode, 
			   char *name)
{
	return co_os_fs_inode_mkdir(filesystem, inode, mode, name);
}

static co_rc_t inode_unlink(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	return co_os_fs_inode_unlink(filesystem, inode, name);
}

static co_rc_t inode_rmdir(co_filesystem_t *filesystem, co_inode_t *inode, char *name)
{
	return co_os_fs_inode_rmdir(filesystem, inode, name);
}

static co_rc_t inode_set_attr(co_filesystem_t *filesystem, co_inode_t *inode,
			      unsigned long valid, struct fuse_attr *attr)
{
	return co_os_fs_inode_set_attr(filesystem, inode, valid, attr);
}

static co_rc_t inode_rename(co_filesystem_t *filesystem, co_inode_t *dir,
			    int new_dir_num, char *oldname, char *newname)
{
	static co_inode_t *new_dir_inode;
	co_rc_t rc;

	new_dir_inode = ino_num_to_inode(new_dir_num, filesystem);
	if (!new_dir_inode) 
		return CO_RC(ERROR);
	rc = co_os_fs_inode_rename(filesystem, dir, new_dir_inode, oldname, newname);;
	if (CO_OK(rc)) {
		co_inode_t *old_inode = find_inode(filesystem, dir, oldname);
		if (old_inode)
			reparent_inode(old_inode, new_dir_inode);
	}
	return rc;
}


static co_rc_t inode_dir_read(co_monitor_t *cmon,
			      co_inode_t *inode, 
			      vm_ptr_t buff, 
			      unsigned long size,
			      unsigned long *fill_size,
			      unsigned long file_pos)
{
	co_filesystem_name_t *name;
	unsigned long file_pos_seek = 0;
	struct fuse_dirent dirent;
	unsigned long dirent_size;
	vm_ptr_t buff_writeptr; 
	co_rc_t rc;

	if (!inode)
		return CO_RC(ERROR);

	if (!inode->names)
		return CO_RC(ERROR);

	buff_writeptr = buff;
	*fill_size = 0;

        co_list_each_entry(name, &inode->names->list, node) {
		int slen = co_strlen(name->name);
		
		dirent_size = FUSE_DIRENT_ALIGN(FUSE_NAME_OFFSET + slen);
		if (file_pos_seek < file_pos) {
			file_pos_seek += dirent_size;
			continue;
		}

		if (dirent_size + *fill_size > size) {
			/* No space left in buffer */
			break;
		}

		if (co_strcmp(name->name, "..") == 0) {
			if (inode->parent)
				dirent.ino = (int)inode->parent;
			else
				dirent.ino = (int)inode;
		} else if (co_strcmp(name->name, ".") == 0) {
			dirent.ino = (int)inode;
		} else {
			dirent.ino = -1;
		}

		dirent.namelen = slen;
		dirent.type = name->type;
		
		co_memcpy(dirent.name, name->name, slen);

		rc = co_monitor_host_to_linuxvm(cmon, &dirent, buff_writeptr, dirent_size);
		if (!CO_OK(rc))
			return rc;
		
		buff_writeptr += dirent_size;
		*fill_size += dirent_size;
		file_pos_seek += dirent_size;
	}

	return CO_RC(OK);
}

static co_rc_t inode_dir_release(co_inode_t *inode)
{
	if (!inode) {
		return CO_RC(ERROR);
	}

	if (!inode->names)
		return CO_RC(ERROR);

	inode->names->refcount--;
	if (inode->names->refcount > 0) 
		return CO_RC(OK);

	co_filesystem_getdir_free(inode->names);
	inode->names = NULL;

	return CO_RC(OK);
}

void co_filesystem_getdir_free(co_filesystem_dir_names_t *names)
{
	co_filesystem_name_t *name;

	while (!co_list_empty(&names->list)) {
		co_list_entry_assign(names->list.next, name, node);
		co_list_del(&name->node);
		co_os_free(name);
	}
}

co_rc_t co_monitor_file_system_init(co_monitor_t *cmon, unsigned long unit, 
				    co_cofsdev_desc_t *desc)
{
	co_filesystem_t *filesystem;

	filesystem = co_os_malloc(sizeof(*filesystem));
	if (!filesystem)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(filesystem, 0, sizeof(*filesystem));

	co_list_init(&filesystem->list_inodes);
	filesystem->desc = desc;
	filesystem->root = alloc_inode(filesystem, NULL, NULL);

	if (!filesystem->root) {
		co_os_free(filesystem);
		return CO_RC(OUT_OF_MEMORY);
	}

	cmon->filesystems[unit] = filesystem;
		
	return CO_RC(OK);
}

void co_monitor_file_system_free(co_monitor_t *cmon, int unit)
{
	co_filesystem_t *filesystem = cmon->filesystems[unit];
	co_inode_t *inode;

	if (!filesystem)
		return;

	while (!co_list_empty(&filesystem->list_inodes)) {		
		co_list_entry_assign(filesystem->list_inodes.next, inode, flat_node);
		free_inode(filesystem, inode);
	}

	co_os_free(filesystem);
	cmon->filesystems[unit] = NULL;
	
}

static co_rc_t inode_get_attr(co_filesystem_t *filesystem, co_inode_t *inode, 
			      struct fuse_getattr_out *attr)
{
	co_rc_t rc;

	rc = co_os_fs_getattr(filesystem, inode, NULL, &attr->attr);

	return rc;
}

static co_rc_t inode_lookup(co_filesystem_t *filesystem, co_inode_t *dir, 
			    char *name, struct fuse_lookup_out *args)
{
	co_rc_t rc = co_os_fs_getattr(filesystem, dir, name, &args->attr);

	if (CO_OK(rc)) {
		co_inode_t *inode = NULL;
		inode = find_inode(filesystem, dir, name);
		if (!inode)
			inode = alloc_inode(filesystem, dir, name);
		args->ino = (unsigned long)inode;
	}

	return rc;
}

static int translate_code(co_rc_t value)
{
	switch (CO_RC_GET_CODE(value)) {
	case CO_RC_NOT_FOUND:
		return -ENOENT;
	case CO_RC_ACCESS_DENIED:
		return -EPERM;
	case CO_RC_INVALID_PARAMETER:
		return -EINVAL;
	case 0:
		return 0;
	default:
		return -EIO;
	}
	return 0;
}

void co_monitor_file_system(co_monitor_t *cmon, unsigned long unit, 
			    enum fuse_opcode opcode, unsigned long *params)
{
	int ino = -1;
	co_filesystem_t *filesystem;
	co_inode_t *inode;
	int result = 0;

	filesystem = cmon->filesystems[unit];
	if (!filesystem)
		return;

	ino = params[0];
	inode = ino_num_to_inode(ino, filesystem);

	switch (opcode) {
	case FUSE_READLINK:
	case FUSE_SYMLINK:
	case FUSE_LINK:
	case FUSE_STATFS:
	case FUSE_RELEASE:
	case FUSE_INVALIDATE:
	case FUSE_FSYNC:
	case FUSE_RELEASE2: 
		break;

	case FUSE_SETATTR: {
		result = inode_set_attr(filesystem, inode, 
					co_passage_page->params[5],
					(struct fuse_attr *)(&co_passage_page->params[6]));
		result = translate_code(result);
		break;
	}

	case FUSE_RENAME: {
		char *str = (char *)&co_passage_page->params[30];
		result = inode_rename(filesystem, inode, 
				      co_passage_page->params[5],
				      str,
				      str + co_strlen(str) + 1);
		result = translate_code(result);
		break;
	}

	case FUSE_FORGET: {
		inode_forget(filesystem, inode);
		break;
	}

	case FUSE_MKNOD:
		result = inode_mknod(filesystem, inode, 
				     co_passage_page->params[5],
				     co_passage_page->params[6],
				     (char *)&co_passage_page->params[30],
				     (int *)&co_passage_page->params[7],
				     (struct fuse_attr *)(&co_passage_page->params[8]));
		result = translate_code(result);
		break;

	case FUSE_MKDIR:
		result = inode_mkdir(filesystem, inode, 
				      co_passage_page->params[5],
				      (char *)&co_passage_page->params[30]);
		result = translate_code(result);
		break;

	case FUSE_UNLINK:
		result = inode_unlink(filesystem, inode, 
				      (char *)&co_passage_page->params[30]);
		result = translate_code(result);
		break;

	case FUSE_RMDIR: 
		result = inode_rmdir(filesystem, inode, 
				     (char *)&co_passage_page->params[30]);
		result = translate_code(result);
		break;

	case FUSE_WRITE: {
		result = inode_write(cmon, filesystem, inode, 
				     *((unsigned long long *)&co_passage_page->params[5]),
				     co_passage_page->params[7],
				     co_passage_page->params[8]);
		result = translate_code(result);
		break;
	}

	case FUSE_READ: {
		result = inode_read(cmon, filesystem, inode, 
				    *((unsigned long long *)&co_passage_page->params[5]),
				    co_passage_page->params[7],
				    co_passage_page->params[8]);
		result = translate_code(result);
		break;
	}

	case FUSE_OPEN: {
		result = inode_open(filesystem, inode, co_passage_page->params[5]);
		result = translate_code(result);
		break;
	}

	case FUSE_LOOKUP: {
		result = 
			inode_lookup(filesystem, inode,
				     (char *)&co_passage_page->params[30],
				     (struct fuse_lookup_out *)&co_passage_page->params[5]);
		result = translate_code(result);
		break;
	}
	case FUSE_GETATTR: {
		result = inode_get_attr(filesystem, inode,
					(struct fuse_getattr_out *)&co_passage_page->params[5]);
		result = translate_code(result);
		break;
	}

	case FUSE_DIR_OPEN:
		result = inode_dir_open(filesystem, inode);
		break;

	case FUSE_DIR_READ:
		result = inode_dir_read(cmon, inode, 
					co_passage_page->params[6],
					co_passage_page->params[5], 
					&co_passage_page->params[7],
					co_passage_page->params[8]);
		break;

	case FUSE_DIR_RELEASE:
		result = inode_dir_release(inode);
		break;

	case FUSE_GETDIR:
		break;
	}	

	co_passage_page->params[4] = result;
}

void co_monitor_unregister_filesystems(co_monitor_t *cmon)
{
	int i;

	for (i=0; i < CO_MODULE_MAX_COFS; i++)
		co_monitor_file_system_free(cmon, i);
}
