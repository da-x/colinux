/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#include <string.h>
#include <mxml.h>

#include <colinux/common/config.h>

co_rc_t co_load_config_blockdev(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	long index = -1;
	char *path = "";
	char *enabled = NULL;
	co_block_dev_desc_t *blockdev;

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];

		if (strcmp(attr->name, "index") == 0)
			index = atoi(attr->value);

		if (strcmp(attr->name, "path") == 0)
			path = attr->value;

		if (strcmp(attr->name, "enabled") == 0)
			enabled = attr->value;
	}
	
	if (index < 0) {
		co_debug("Invalid block_dev element: bad index\n");
		return CO_RC(ERROR);
	}

	if (index >= CO_MAX_BLOCK_DEVICES) {
		co_debug("Invalid block_dev element: bad index\n");
		return CO_RC(ERROR);
	}

	if (path == NULL) {
		co_debug("Invalid block_dev element: bad path\n");
		return CO_RC(ERROR);
	}

	blockdev = &out_config->block_devs[index];
	
	snprintf(blockdev->pathname, sizeof(blockdev->pathname), "%s", path);
	blockdev->enabled = enabled ? (strcmp(enabled, "true") == 0) : 0;

	return CO_RC(OK);
}

co_rc_t co_load_config_image(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	char *path = "";

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];

		if (strcmp(attr->name, "path") == 0)
			path = attr->value;
	}
	
	if (path == NULL) {
		co_debug("Invalid image element: bad path\n");
		return CO_RC(ERROR);
	}

	snprintf(out_config->vmlinux_path, sizeof(out_config->vmlinux_path), 
		 "%s", path);

	return CO_RC(OK);
}

co_rc_t co_load_config_boot_params(co_config_t *out_config, mxml_node_t *node)
{
	char *param_line;
	unsigned long param_line_size_left;
	unsigned long index;
	mxml_node_t *text_node;

	if (node == NULL)
		return CO_RC(ERROR);

	param_line = out_config->boot_parameters_line;
	param_line_size_left = sizeof(out_config->boot_parameters_line);

	bzero(param_line, param_line_size_left);

	text_node = node;
	index = 0;

	while (text_node  &&  text_node->type == MXML_TEXT) {
		if (index != 0) {
			int param_size = strlen(param_line);
			param_line += param_size;
			param_line_size_left -= param_size;
		}
						
		snprintf(param_line, 
			 param_line_size_left, 
			 index == 0 ? "%s" : " %s", 
			 text_node->value.text.string);

		index++;
		text_node = text_node->next;
	}

	return CO_RC(OK);
}

co_rc_t co_load_config_memory(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	char *size = "";

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];

		if (strcmp(attr->name, "size") == 0)
			size = attr->value;
	}
	
	if (size == NULL) {
		co_debug("Invalid memory element: bad size\n");
		return CO_RC(ERROR);
	}

	out_config->ram_size = atoi(size);

	return CO_RC(OK);
}

co_rc_t co_load_config(char *text, co_config_t *out_config)
{
	mxml_node_t *node, *tree, *walk;
	char *name;

	tree = mxmlLoadString(NULL, text, NULL);

	node = mxmlFindElement(tree, tree, "colinux", NULL, NULL,
			       MXML_DESCEND);

	for (walk = node->child; walk; walk = walk->next) {
		if (walk->type == MXML_ELEMENT) {
			name = walk->value.element.name;
			if (strcmp(name, "block_device") == 0) {
				co_load_config_blockdev(out_config, &walk->value.element);
			} else if (strcmp(name, "bootparams") == 0) {
				co_load_config_boot_params(out_config, walk->child);
			} else if (strcmp(name, "image") == 0) {
				co_load_config_image(out_config, &walk->value.element);
			} else if (strcmp(name, "memory") == 0) {
				co_load_config_memory(out_config, &walk->value.element);
			}
		}
	}

	mxmlRemove(tree);

	if (strcmp(out_config->vmlinux_path, "") == 0)
		snprintf(out_config->vmlinux_path, sizeof(out_config->vmlinux_path), "vmlinux");

	return CO_RC(OK);
}
