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
		cobd->enabled = PTRUE;

		co_snprintf(cobd->pathname, sizeof(cobd->pathname), "%s", param);
		co_canonize_cobd_path(&cobd->pathname);
		co_terminal_print("mapping cobd%d to %s\n", index, cobd->pathname);
	} while (1);

	return CO_RC(OK);
}

static co_rc_t allocate_by_alias(co_config_t *conf, const char *prefix, const char *suffix, 
				 const char *param)
{
	co_block_dev_desc_t *cobd;
	int i;

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
	cobd->alias_used = PTRUE;
	co_snprintf(cobd->alias, sizeof(cobd->alias), "%s%s", prefix, suffix);

	co_canonize_cobd_path(&cobd->pathname);

	co_terminal_print("selected cobd%d for %s, mapping to '%s'\n", i, cobd->alias, cobd->pathname);

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
					return CO_RC(ERROR);
				}

				if (index < 0 || index >= CO_MODULE_MAX_COBD) {
					co_terminal_print("invalid cobd index %d in alias %s%s\n", index, prefix, suffix);
					return CO_RC(ERROR);
				}

				cobd = &conf->block_devs[index];
				if (!cobd->enabled) {
					co_terminal_print("warning alias on disabled cobd%d\n", index);
				}
				
				if (cobd->alias_used) {
					co_terminal_print("error, alias cannot be used twice for cobd%d\n", index);
					return CO_RC(ERROR);
				}

				cobd->alias_used = PTRUE;
				co_snprintf(cobd->alias, sizeof(cobd->alias), "%s%s", prefix, suffix);

				co_terminal_print("mapping %s%s to %s\n", prefix, suffix, &param[1]);
				
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

	if (*mac_address) {
		rc = co_parse_mac_address(mac_address, net_dev->mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", mac_address);
			return rc;
		}
		net_dev->manual_mac_address = PTRUE;
	}

	co_terminal_print("configured TAP at '%s' device as eth%d\n",
				net_dev->desc, index);

	if (*mac_address)
		co_terminal_print("MAC address: %s\n", mac_address);
	else
		co_terminal_print("MAC address: auto generated\n");

	if (*host_ip)
		co_terminal_print("Host IP address: %s (currently ignored)\n", host_ip);

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

	if (*mac_address) {
		rc = co_parse_mac_address(mac_address, net_dev->mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", mac_address);
			return rc;
		}
		net_dev->manual_mac_address = PTRUE;
	}

	co_terminal_print("configured PCAP bridge at '%s' device as eth%d\n", 
			net_dev->desc, index);

	if (*mac_address)
		co_terminal_print("MAC address: %s\n", mac_address);
	else
		co_terminal_print("MAC address: auto generated\n");

	if (strlen(promisc_mode) > 0) {
		if (strcmp(promisc_mode, "nopromisc") == 0) {
			net_dev->promisc_mode = 0;
			co_terminal_print("Pcap mode: nopromisc\n");
		} else if (strcmp(promisc_mode, "promisc") == 0) {
			net_dev->promisc_mode = 1;
			co_terminal_print("Pcap mode: promisc\n");
		} else {
			co_terminal_print("error: PCAP bridge option only allowed 'promisc' or 'nopromisc'\n");
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_slirp(co_config_t *conf, int index, const char *param)
{
	co_netdev_desc_t *net_dev = &conf->net_devs[index];
	char mac_address[40]; /* currently ignored */

	comma_buffer_t array [] = {
		{ sizeof(mac_address), mac_address },
		{ sizeof(net_dev->redir), net_dev->redir },
		{ 0, NULL }
	};

	split_comma_separated(param, array);

	net_dev->type = CO_NETDEV_TYPE_SLIRP;
	net_dev->enabled = PTRUE;

	co_terminal_print("configured Slirp as eth%d\n", index);

	if (*net_dev->redir)
		co_terminal_print("redirections %s\n", net_dev->redir);

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
		return CO_RC(ERROR);
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

		rc = parse_args_networking_device(conf, index, param);
		if (!CO_OK(rc))
			return rc;
	} while (1);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_cofs(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char *param;
	co_rc_t rc;

	do {
		int index;
		co_cofsdev_desc_t *cofs;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "cofs", 
							     &index, CO_MODULE_MAX_COFS, 
							     &param, &exists);
		if (!CO_OK(rc))
			return rc;
		
		if (!exists)
			break;

		cofs = &conf->cofs_devs[index];
		cofs->enabled = PTRUE;

		co_snprintf(cofs->pathname, sizeof(cofs->pathname), "%s", param);
		co_canonize_cobd_path(&cofs->pathname);
		co_terminal_print("mapping cofs%d to %s\n", index, cofs->pathname);
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

	serial->enabled = PTRUE;

	split_comma_separated(param, array);

	if (!*name) {
		co_terminal_print("missing host serial device name for ttys%d\n", index);
		return CO_RC(INVALID_PARAMETER);
	}

	co_terminal_print("mapping ttys%d to %s\n", index, name);
	serial->desc = strdup(name);
	
	if (*mode) {
		co_debug("mode: %s", mode);
		serial->mode = strdup(mode);
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

	execute->enabled = PTRUE;
	execute->pid = 0;

	split_comma_separated(param, array);

	if (!*prog) {
		co_terminal_print("missing program path for exec%d\n", index);
		return CO_RC(INVALID_PARAMETER);
	}

	co_debug("exec%d: '%s'", index, prog);
	execute->prog = strdup(prog);
	
	if (*args) {
		co_debug("args%d: %s", index, args);
		execute->args = strdup(args);
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

	rc = co_cmdline_get_next_equality(cmdline, "initrd", 0, NULL, 0, 
					  conf->initrd_path, sizeof(conf->initrd_path),
					  &conf->initrd_enabled);
	if (!CO_OK(rc)) 
		return rc;

	if (conf->initrd_enabled) {
		co_remove_quotation_marks(conf->initrd_path);
		co_terminal_print("using '%s' as initrd image\n", conf->initrd_path);
	}

	rc = co_cmdline_get_next_equality_int_value(cmdline, "mem", (int *)&conf->ram_size, &exists);
	if (!CO_OK(rc)) 
		return rc;

	if (!exists)
		conf->ram_size = 32;
		
	co_terminal_print("configuring %d MB of virtual RAM\n", conf->ram_size);

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
	co_terminal_print("using '%s' as kernel image\n", conf->vmlinux_path);

	rc = parse_config_args(cmdline, conf);
	if (!CO_OK(rc))
		co_terminal_print("daemon: error parsing configuration parameters\n");
	
	rc_ = co_cmdline_params_format_remaining_parameters(cmdline, conf->boot_parameters_line,
							    sizeof(conf->boot_parameters_line));
	if (!CO_OK(rc_))
		return rc_;
	
	if (CO_OK(rc)) {
		co_terminal_print("kernel boot parameters: '%s'\n", conf->boot_parameters_line);
		co_terminal_print("\n");
		start_parameters->config_specified = PTRUE;
	}

	return rc;
}
