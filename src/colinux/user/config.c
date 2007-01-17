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

#include <colinux/common/libc.h>
#include <colinux/common/config.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/cobdpath.h>
#include <colinux/os/alloc.h>
#include "macaddress.h"

#include "daemon.h"

typedef struct {
    int size;
    char * buffer;
    } comma_buffer_t;	/* only for split_comma_separated here */

/* 
 * "New" command line configuration gathering scheme. It existed a long time in 
 * User Mode Linux and should be less hussle than handling XML files for the
 * novice users.
 */

static co_rc_t check_cobd_file (co_pathname_t pathname, int index)
{
	co_rc_t rc;
	char *buf;
	unsigned long size;
	static const char magic_bz2 [3] = "BZh";		/* bzip2 compressed data */
	static const char magic_7z  [6] = "7z\274\257\047\034";	/* 7z archive data */

	if (co_global_debug_levels.misc_level < 2)
		return CO_RC(OK);  /* verbose is not enabled */

	co_remove_quotation_marks(pathname);

	if (co_strncmp(pathname, "\\Device\\", 8) == 0  ||
	    co_strncmp(pathname, "\\DosDevices\\", 12) == 0 ||
	    co_strncmp(pathname, "\\Volume{", 8) == 0 ||
	    co_strncmp(pathname, "\\\\.\\", 4) == 0) 
		return CO_RC(OK);  /* Can't check partitions or raw devices */

	rc = co_os_file_load(pathname, &buf, &size, 1024);
	if (!CO_OK(rc)) {
		co_terminal_print("cobd%d: error reading file\n", index);
		return rc;
	}

	if (size != 1024) {
		co_terminal_print("cobd%d: file to small (%s)\n", index, pathname);
		rc = CO_RC(INVALID_PARAMETER);
	} else
	if (memcmp(buf, magic_bz2, sizeof(magic_bz2)) == 0 ||
	    memcmp(buf, magic_7z, sizeof(magic_7z)) == 0) {
		co_terminal_print("cobd%d: Image file must be unpack before (%s)\n", index, pathname);
		rc = CO_RC(INVALID_PARAMETER);
	}

	co_os_file_free(buf);
	return rc;
}

static co_rc_t parse_args_config_cobd(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		int index;
		co_block_dev_desc_t *cobd;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "cobd", 
							     &index, CO_MODULE_MAX_COBD, 
							     &param, &exists);
		if (!CO_OK(rc))
			return rc;
		
		if (!exists)
			break;

		cobd = &conf->block_devs[index];

		if (cobd->enabled) {
			co_terminal_print("cobd%d double defined\n", index);
			return CO_RC(INVALID_PARAMETER);
		}
		cobd->enabled = PTRUE;

		co_snprintf(cobd->pathname, sizeof(cobd->pathname), "%s", param);

		rc = check_cobd_file(cobd->pathname, index);
		if (!CO_OK(rc))
			return rc;

		co_canonize_cobd_path(&cobd->pathname);
		co_debug_info("mapping cobd%d to %s", index, cobd->pathname);

	} while (1);

	return CO_RC(OK);
}

static co_rc_t allocate_by_alias(co_config_t *conf, const char *prefix, const char *suffix, 
				 const char *param)
{
	co_block_dev_desc_t *cobd;
	int i;
	co_rc_t rc;

	for (i=0; i < CO_MODULE_MAX_COBD; i++) {
		cobd = &conf->block_devs[i];
		
		if (!cobd->enabled)
			break;
	}

	if (i == CO_MODULE_MAX_COBD) {
		co_terminal_print("no available cobd for new alias\n");
		return CO_RC(ERROR);
	}

	cobd->enabled = PTRUE;
	co_snprintf(cobd->pathname, sizeof(cobd->pathname), "%s", param);

	rc = check_cobd_file (cobd->pathname, i);
	if (!CO_OK(rc))
		return rc;

	co_canonize_cobd_path(&cobd->pathname);

	cobd->alias_used = PTRUE;
	co_snprintf(cobd->alias, sizeof(cobd->alias), "%s%s", prefix, suffix);

	co_debug_info("selected cobd%d for %s, mapping to '%s'", i, cobd->alias, cobd->pathname);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_aliases(co_command_line_params_t cmdline, co_config_t *conf)
{
	const char *prefixes[] = {"sd", "hd"};
	const char *prefix;
	bool_t exists;
	char *param;
	co_rc_t rc;
	int i;

	for (i=0; i < sizeof(prefixes)/sizeof(char *); i++) {
		prefix = prefixes[i];
		char suffix[5];
			
		do {
			rc = co_cmdline_get_next_equality_alloc(cmdline, prefix, sizeof(suffix)-1, suffix, sizeof(suffix), 
							  &param, &exists);
			if (!CO_OK(rc)) 
				return rc;
			
			if (!exists)
				break;

 			if (!co_strncmp(":cobd", param, 5)) {
				char *index_str = &param[5];
				char *number_parse  = NULL;
				int index;
				co_block_dev_desc_t *cobd;
				
				index = co_strtol(index_str, &number_parse, 10);
				if (number_parse == index_str) {
					co_terminal_print("invalid alias: %s%s=%s\n", prefix, suffix, param);
					return CO_RC(INVALID_PARAMETER);
				}

				if (index < 0 || index >= CO_MODULE_MAX_COBD) {
					co_terminal_print("invalid cobd index %d in alias %s%s\n", index, prefix, suffix);
					return CO_RC(INVALID_PARAMETER);
				}

				cobd = &conf->block_devs[index];
				if (!cobd->enabled) {
					co_terminal_print("alias on unused cobd%d\n", index);
					return CO_RC(INVALID_PARAMETER);
				}
				
				if (cobd->alias_used) {
					co_terminal_print("error, alias cannot be used twice for cobd%d\n", index);
					return CO_RC(INVALID_PARAMETER);
				}

				cobd->alias_used = PTRUE;
				co_snprintf(cobd->alias, sizeof(cobd->alias), "%s%s", prefix, suffix);

				co_debug_info("mapping %s%s to %s", prefix, suffix, &param[1]);
				
			} else {
				rc = allocate_by_alias(conf, prefix, suffix, param);
				if (!CO_OK(rc))
					return rc;
			}
		} while (1);
	}

	return CO_RC(OK);
}

static bool_t strmatch_identifier(const char *str, const char *identifier, const char **end)
{
	while (*str == *identifier  &&  *str != '\0') {
		str++;
		identifier++;
	}

	if (end)
		*end = str;

	if (*identifier != '\0')
		return PFALSE;

	if (!((*str >= 'a'  &&  *str <= 'z') || (*str >= 'A'  &&  *str <= 'Z'))) {
		if (end) {
			if (*str != '\0')
				(*end)++;
		}
		return PTRUE;
	}

	return PFALSE;
}

static void split_comma_separated(const char *source, comma_buffer_t *array)
{
	int j;
	bool_t quotation_marks;
	
	for (; array->buffer != NULL; array++) {
		j = 0;
		quotation_marks = PFALSE;

		// quotation marks detection in config file, sample:
		// eth0=tuntap,"LAN-Connection 14",00:11:22:33:44:55
		// String store without quotation marks.

		while (j < array->size - 1 && *source != '\0') {
			if (*source == '"') {
				if (*++source != '"') {
					quotation_marks = !quotation_marks;
					continue;
				}
			} else if (*source == ',' && !quotation_marks)
				break;

			array->buffer[j++] = *source++;
		}

		// End for destination buffers
		array->buffer[j] = '\0';

		// Skip, if end of source, but go away for _all_ buffers
		if (*source != '\0')
			source++;
	}
}

static co_rc_t config_parse_mac_address(const char *text, co_netdev_desc_t *net_dev)
{
	co_rc_t rc;

	if (*text) {
		rc = co_parse_mac_address(text, net_dev->mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", text);
			return rc;
		}
		net_dev->manual_mac_address = PTRUE;

		co_debug_info("MAC address: %s\n", text);
	} else {
		co_debug("MAC address: auto generated");
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_tap(co_config_t *conf, int index, const char *param)
{
	co_netdev_desc_t *net_dev = &conf->net_devs[index];
	char mac_address[40];
	char host_ip[40];
	co_rc_t rc;

	comma_buffer_t array [] = {
		{ sizeof(net_dev->desc), net_dev->desc },
		{ sizeof(mac_address), mac_address },
		{ sizeof(host_ip), host_ip },
		{ 0, NULL }
	};

	split_comma_separated(param, array);

	net_dev->type = CO_NETDEV_TYPE_TAP;
	net_dev->enabled = PTRUE;

	co_debug_info("configured TAP at '%s' device as eth%d",
				net_dev->desc, index);

	rc = config_parse_mac_address(mac_address, net_dev);
	if (!CO_OK(rc))
		return rc;

	if (*host_ip)
		co_debug_info("Host IP address: %s (currently ignored)", host_ip);

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_pcap(co_config_t *conf, int index, const char *param)
{
	co_netdev_desc_t *net_dev = &conf->net_devs[index];
	char mac_address[40];
	char promisc_mode[10];
	co_rc_t rc;

	comma_buffer_t array [] = {
		{ sizeof(net_dev->desc), net_dev->desc },
		{ sizeof(mac_address), mac_address },
		{ sizeof(promisc_mode), promisc_mode },
		{ 0, NULL }
	};

	split_comma_separated(param, array);

	net_dev->type = CO_NETDEV_TYPE_BRIDGED_PCAP;
	net_dev->enabled = PTRUE;
	net_dev->promisc_mode = 1;

	co_debug_info("configured PCAP bridge at '%s' device as eth%d",
			net_dev->desc, index);

	rc = config_parse_mac_address(mac_address, net_dev);
	if (!CO_OK(rc))
		return rc;

	if (*promisc_mode) {
		if (strcmp(promisc_mode, "nopromisc") == 0) {
			net_dev->promisc_mode = 0;
			co_debug_info("Pcap mode: nopromisc");
		} else if (strcmp(promisc_mode, "promisc") == 0) {
			net_dev->promisc_mode = 1;
			co_debug_info("Pcap mode: promisc");
		} else {
			co_terminal_print("error: PCAP bridge option only allowed 'promisc' or 'nopromisc'\n");
			return CO_RC(INVALID_PARAMETER);
		}
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_slirp(co_config_t *conf, int index, const char *param)
{
	co_netdev_desc_t *net_dev = &conf->net_devs[index];
	char mac_address[40];
	co_rc_t rc;

	comma_buffer_t array [] = {
		{ sizeof(mac_address), mac_address },
		{ sizeof(net_dev->redir), net_dev->redir },
		{ 0, NULL }
	};

	split_comma_separated(param, array);

	net_dev->type = CO_NETDEV_TYPE_SLIRP;
	net_dev->enabled = PTRUE;

	co_debug_info("configured Slirp as eth%d", index);

	rc = config_parse_mac_address(mac_address, net_dev);
	if (!CO_OK(rc))
		return rc;

	if (*net_dev->redir)
		co_debug_info("redirections %s", net_dev->redir);

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device(co_config_t *conf, int index, const char *param)
{
	const char *next = NULL;

	if (strmatch_identifier(param, "tuntap", &next)) {
		return parse_args_networking_device_tap(conf, index, next);
	} else if (strmatch_identifier(param, "pcap-bridge", &next)) {
		return parse_args_networking_device_pcap(conf, index, next);
	} else if (strmatch_identifier(param, "slirp", &next)) {
		return parse_args_networking_device_slirp(conf, index, next);
	} else {
		co_terminal_print("unsupported network transport type: %s\n", param);
		co_terminal_print("supported types are: tuntap, pcap-bridge, slirp\n");
		return CO_RC(INVALID_PARAMETER);
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		int index;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "eth", 
							     &index, CO_MODULE_MAX_CONET, 
							     &param, &exists);
		if (!CO_OK(rc))
			return rc;
		
		if (!exists)
			break;

		if (conf->net_devs[index].enabled) {
			co_terminal_print("eth%d double defined\n", index);
			return CO_RC(INVALID_PARAMETER);
		}

		rc = parse_args_networking_device(conf, index, param);
		if (!CO_OK(rc))
			return rc;
	} while (1);

	return CO_RC(OK);
}

static co_rc_t parse_args_cofs_device(co_config_t *conf, int index, const char *param)
{
	co_cofsdev_desc_t *cofs = &conf->cofs_devs[index];

	if (cofs->enabled) {
		co_terminal_print("warning cofs%d double defined\n", index);
		return CO_RC(INVALID_PARAMETER);
	}
	cofs->enabled = PTRUE;

	co_snprintf(cofs->pathname, sizeof(cofs->pathname), "%s", param);
	co_canonize_cobd_path(&cofs->pathname);
	co_debug_info("mapping cofs%d to %s", index, cofs->pathname);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_cofs(co_command_line_params_t cmdline, co_config_t *conf)
{
	int index;
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "cofs", 
							     &index, CO_MODULE_MAX_COFS, 
							     &param, &exists);
		if (!CO_OK(rc))
			return rc;
		
		if (!exists)
			break;

		rc = parse_args_cofs_device(conf, index, param);
		if (!CO_OK(rc))
			return rc;
	} while (1);

	return CO_RC(OK);
}

static co_rc_t parse_args_serial_device(co_config_t *conf, int index, const char *param)
{
	co_serialdev_desc_t *serial = &conf->serial_devs[index];
	char name [CO_SERIAL_DESC_STR_SIZE];
	char mode [CO_SERIAL_MODE_STR_SIZE];

	comma_buffer_t array [] = {
		{ sizeof(name), name },
		{ sizeof(mode), mode },
		{ 0, NULL }
	};

	if (serial->enabled) {
		co_terminal_print("ttys%d double defined\n", index);
		return CO_RC(INVALID_PARAMETER);
	}
	serial->enabled = PTRUE;

	split_comma_separated(param, array);

	if (!*name) {
		co_terminal_print("missing host serial device name for ttys%d\n", index);
		return CO_RC(INVALID_PARAMETER);
	}

	co_debug_info("mapping ttys%d to %s", index, name);
	serial->desc = strdup(name);
	if (!serial->desc)
		return CO_RC(OUT_OF_MEMORY);
	
	if (*mode) {
		co_debug("mode: %s", mode);
		serial->mode = strdup(mode);
		if (!serial->mode)
			return CO_RC(OUT_OF_MEMORY);
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_config_serial(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		int index;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "ttys", 
							     &index, CO_MODULE_MAX_SERIAL, 
							     &param, &exists);
		if (!CO_OK(rc)) 
			return rc;

		if (!exists)
			break;

		rc = parse_args_serial_device(conf, index, param);
		if (!CO_OK(rc)) 
			return rc;
	} while (1);

	return CO_RC(OK);
}

static co_rc_t parse_args_execute(co_config_t *conf, int index, const char *param)
{
	co_execute_desc_t *execute = &conf->executes[index];
	char prog [CO_EXECUTE_PROG_STR_SIZE];
	char args [CO_EXECUTE_ARGS_STR_SIZE];

	comma_buffer_t array [] = {
		{ sizeof(prog), prog },
		{ sizeof(args), args },
		{ 0, NULL }
	};

	if (execute->enabled) {
		co_terminal_print("exec%d double defined\n", index);
		return CO_RC(INVALID_PARAMETER);
	}
	execute->enabled = PTRUE;
	execute->pid = 0;

	split_comma_separated(param, array);

	if (!*prog) {
		co_terminal_print("missing program path for exec%d\n", index);
		return CO_RC(INVALID_PARAMETER);
	}

	co_debug("exec%d: '%s'", index, prog);
	execute->prog = strdup(prog);
	if (!execute->prog)
		return CO_RC(OUT_OF_MEMORY);
	
	if (*args) {
		co_debug("args%d: %s", index, args);
		execute->args = strdup(args);
		if (!execute->args)
			return CO_RC(OUT_OF_MEMORY);
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_config_execute(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		int index;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "exec", 
							     &index, CO_MODULE_MAX_EXECUTE, 
							     &param, &exists);
		if (!CO_OK(rc)) 
			return rc;

		if (!exists)
			break;

		rc = parse_args_execute(conf, index, param);
		if (!CO_OK(rc)) 
			return rc;
	} while (1);

	return CO_RC(OK);
}

static co_rc_t parse_config_args(co_command_line_params_t cmdline, co_config_t *conf)
{
	co_rc_t rc;
	bool_t exists;

	rc = co_cmdline_get_next_equality_int_value(cmdline, "mem", (int *)&conf->ram_size, &exists);
	if (!CO_OK(rc)) 
		return rc;

	if (!exists)
		conf->ram_size = 32;
		
	co_debug_info("configuring %d MB of virtual RAM", conf->ram_size);

	rc = parse_args_config_cobd(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_config_aliases(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_networking(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_config_cofs(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_get_next_equality(cmdline, "initrd", 0, NULL, 0, 
					  conf->initrd_path, sizeof(conf->initrd_path),
					  &conf->initrd_enabled);
	if (!CO_OK(rc)) 
		return rc;

	if (conf->initrd_enabled) {
		co_remove_quotation_marks(conf->initrd_path);
		co_debug_info("using '%s' as initrd image", conf->initrd_path);

		/* Is last cofs free for automatic set? */
		if (!conf->cofs_devs[CO_MODULE_MAX_COFS-1].enabled) {
			char *param;

			/* copy path from initrd file */
			param = strdup(conf->initrd_path);
			if (!param)
				return CO_RC(OUT_OF_MEMORY);

			/* get only the directory */
			co_dirname(param);

			if (*param)
				rc = parse_args_cofs_device(conf, CO_MODULE_MAX_COFS-1, param);

			co_os_free (param);

			if (!CO_OK(rc))
				return rc;
		}
	}

	rc = parse_args_config_serial(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_config_execute(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	return rc;
}

co_rc_t co_parse_config_args(co_command_line_params_t cmdline, co_start_parameters_t *start_parameters)
{
	co_rc_t rc, rc_;
	co_config_t *conf = &start_parameters->config;

	start_parameters->cmdline_config = PFALSE;

	rc = co_cmdline_get_next_equality(cmdline, "kernel", 0, NULL, 0, 
					  conf->vmlinux_path, sizeof(conf->vmlinux_path),
					  &start_parameters->cmdline_config);
	if (!CO_OK(rc))
		return rc;

	if (!start_parameters->cmdline_config)
		return CO_RC(OK);

	co_remove_quotation_marks(conf->vmlinux_path);
	co_debug_info("using '%s' as kernel image", conf->vmlinux_path);

	rc = parse_config_args(cmdline, conf);
	
	rc_ = co_cmdline_params_format_remaining_parameters(cmdline, conf->boot_parameters_line,
							    sizeof(conf->boot_parameters_line));
	if (!CO_OK(rc_))
		return rc_;
	
	if (CO_OK(rc)) {
		co_debug_info("kernel boot parameters: '%s'\n", conf->boot_parameters_line);
		co_debug_info("\n");
		start_parameters->config_specified = PTRUE;
	}

	return rc;
}
