/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 
#include <string.h>
#include <mxml.h>

#include <colinux/common/libc.h>
#include <colinux/common/config.h>
#include "macaddress.h"

co_rc_t co_load_config_blockdev(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	long index = -1;
	char *path = "";
	char *alias = NULL;
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

		if (strcmp(attr->name, "alias") == 0)
			alias = attr->value;
	}
	
	if (index < 0) {
		co_debug("config: invalid block_dev element: bad index\n");
		return CO_RC(ERROR);
	}

	if (index >= CO_MODULE_MAX_COBD) {
		co_debug("config: invalid block_dev element: bad index\n");
		return CO_RC(ERROR);
	}

	if (path == NULL) {
		co_debug("config: invalid block_dev element: bad path\n");
		return CO_RC(ERROR);
	}

	blockdev = &out_config->block_devs[index];
	
	snprintf(blockdev->pathname, sizeof(blockdev->pathname), "%s", path);
	blockdev->enabled = enabled ? (strcmp(enabled, "true") == 0) : 0;

	snprintf(blockdev->alias, sizeof(blockdev->alias), "%s", alias);
	blockdev->alias_used = alias != NULL;

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
		co_debug("config: invalid image element: bad path\n");
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

	co_bzero(param_line, param_line_size_left);

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

co_rc_t co_load_config_initrd(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	char *path = "";

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];
                                                     
		if (strcmp(attr->name, "path") == 0)  
			path = attr->value;
	}

	if (path == NULL) {
		co_debug("config: invalid initrd element: bad path\n");
		return CO_RC(ERROR);
	}

	out_config->initrd_enabled = PTRUE;

	snprintf(out_config->initrd_path, sizeof(out_config->initrd_path),
		 "%s", path);

	return CO_RC(OK);
}


static bool_t char_is_digit(char ch)
{
	return (ch >= '0'  &&  ch <= '9');
}

co_rc_t co_str_to_unsigned_long(const char *text, unsigned long *number_out)
{
	unsigned long number = 0;
	unsigned long last_number = 0;

	if (!char_is_digit(*text))
		return CO_RC(ERROR);

	do {
		last_number = number;
		number *= 10;
		number += (*text - '0');

		if (number < last_number) {
			/* Overflow */
			return CO_RC(ERROR);
		}

	} while (char_is_digit(*++text)) ;

	if (*text == '\0') {
		*number_out = number;
		return CO_RC(OK);
	}

	return CO_RC(ERROR);
}

co_rc_t co_load_config_memory(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	char *element_text = NULL;
	co_rc_t rc;

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];

		if (strcmp(attr->name, "size") == 0)
			element_text = attr->value;
	}
	
	if (element_text == NULL) {
		co_debug("config: invalid memory element: bad memory specification\n");
		return CO_RC(ERROR);
	}

	rc = co_str_to_unsigned_long(element_text, &out_config->ram_size);
	if (!CO_OK(rc))
		co_debug("config: invalid memory element: invalid size format\n");

	return rc;
}

co_rc_t co_load_config_network(co_config_t *out_config, mxml_element_t *element)
{
	int i;
	char *element_text = NULL;
	co_rc_t rc = CO_RC(ERROR);
	co_netdev_desc_t desc = {0, };
	unsigned long index = -1;

	desc.enabled = PFALSE;
	desc.type = CO_NETDEV_TYPE_BRIDGED_PCAP;
	desc.manual_mac_address = PFALSE;

	for (i=0; i < element->num_attrs; i++) {
		mxml_attr_t *attr = &element->attrs[i];

		if (strcmp(attr->name, "mac") == 0) {
			element_text = attr->value;

			rc = co_parse_mac_address(element_text, desc.mac_address);
			if (!CO_OK(rc)) { 
				co_debug("config: invalid network element: invalid mac address specified (use the xx:xx:xx:xx:xx:xx format)\n");
				return rc;
			}

			desc.manual_mac_address = PTRUE;
		} else
		if (strcmp(attr->name, "index") == 0) {
			element_text = attr->value;

			rc = co_str_to_unsigned_long(element_text, &index);
			if (!CO_OK(rc)) {
				co_debug("config: invalid network element: invalid index format\n");
				return CO_RC(ERROR);
			}
			
			if (index < 0  ||  index >= CO_MODULE_MAX_CONET) {
				co_debug("config: invalid network element: invalid index %d\n", (int)index);
				return CO_RC(ERROR);
			}

			desc.enabled = PTRUE;
		} else
		if (strcmp(attr->name, "name") == 0) {
			element_text = attr->value;

			snprintf(desc.desc, sizeof(desc.desc), "%s", element_text);
		} else
		if (strcmp(attr->name, "type") == 0) {
			element_text = attr->value;

			if (strcmp(element_text, "bridged") == 0) {
				desc.type = CO_NETDEV_TYPE_BRIDGED_PCAP;
			} else if (strcmp(element_text, "tap") == 0) {
				desc.type = CO_NETDEV_TYPE_TAP;
			} else {
				co_debug("config: invalid network element: invalid type %s\n", element_text);
				return CO_RC(ERROR);
			}
		}
	}

	if (index == -1) {
		co_debug("config: invalid network element: index not specified\n");
		return CO_RC(ERROR);
	}

	out_config->net_devs[index] = desc;
	
	return rc;
}

co_rc_t co_load_config(char *text, co_config_t *out_config)
{
	mxml_node_t *node, *tree, *walk;
	char *name;
	co_rc_t rc = CO_RC(OK);

	out_config->initrd_enabled = PFALSE;

	tree = mxmlLoadString(NULL, text, NULL);
	if (tree == NULL) {
		co_debug("config: error parsing config XML. Please check XML's validity\n");
		return CO_RC(ERROR);
	}

	node = mxmlFindElement(tree, tree, "colinux", NULL, NULL, MXML_DESCEND);

	if (node == NULL) {
		co_debug("config: couldn't find colinux element within XML\n");
		return CO_RC(ERROR);
	}

	for (walk = node->child; walk; walk = walk->next) {
		if (walk->type == MXML_ELEMENT) {
			name = walk->value.element.name;
			rc = CO_RC(OK);

			if (strcmp(name, "block_device") == 0) {
				rc = co_load_config_blockdev(out_config, &walk->value.element);
			} else if (strcmp(name, "bootparams") == 0) {
				rc = co_load_config_boot_params(out_config, walk->child);
			} else if (strcmp(name, "image") == 0) {
				rc = co_load_config_image(out_config, &walk->value.element);
			} else if (strcmp(name, "initrd") == 0) {
				rc = co_load_config_initrd(out_config, &walk->value.element);
			} else if (strcmp(name, "memory") == 0) {
				rc = co_load_config_memory(out_config, &walk->value.element);
			} else if (strcmp(name, "network") == 0) {
				rc = co_load_config_network(out_config, &walk->value.element);
			}

			if (!CO_OK(rc))
				break;
		}
	}

	mxmlRemove(tree);

	if (strcmp(out_config->vmlinux_path, "") == 0)
		snprintf(out_config->vmlinux_path, sizeof(out_config->vmlinux_path), "vmlinux");

	return rc;
}
