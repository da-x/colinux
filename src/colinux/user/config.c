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


/* 
 * "New" command line configuration gathering scheme. It existed a long time in 
 * User Mode Linux and should be less hussle than handling XML files for the
 * novice users.
 */

static co_rc_t parse_args_config_cobd(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char param[0x100];
	co_rc_t rc;

	do {
		int index;
		exists = PFALSE;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "cobd", &index, param, 
							     sizeof(param), &exists);
		if (!CO_OK(rc)) 
			return rc;
		
		if (exists) {
			co_block_dev_desc_t *cobd;

			if (index < 0  || index >= CO_MODULE_MAX_COBD) {
				co_terminal_print("invalid cobd index: %d\n", index);
				return CO_RC(ERROR);
			}

			cobd = &conf->block_devs[index];
			cobd->enabled = PTRUE;

			co_snprintf(cobd->pathname, sizeof(cobd->pathname), "%s", param);

			co_canonize_cobd_path(&cobd->pathname);

			co_terminal_print("mapping cobd%d to %s\n", index, cobd->pathname);
		}
	} while (exists);

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

	co_terminal_print("selected cobd%d for %s%s, mapping to '%s'\n", i, prefix, suffix, cobd->pathname);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_aliases(co_command_line_params_t cmdline, co_config_t *conf)
{
	const char *prefixes[] = {"sd", "hd"};
	const char *prefix;
	bool_t exists;
	char param[0x100];
	co_rc_t rc;
	int i;

	for (i=0; i < sizeof(prefixes)/sizeof(char *); i++) {
		prefix = prefixes[i];
		char suffix[5];
			
		do {
			rc = co_cmdline_get_next_equality(cmdline, prefix, sizeof(suffix)-1, suffix, sizeof(suffix), 
							  param, sizeof(param), &exists);
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

				if (index < 0  || index >= CO_MODULE_MAX_COBD) {
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

				co_terminal_print("mapping %s%s to param\n", prefix, suffix, &param[1]);
				
			} else {
				rc = allocate_by_alias(conf, prefix, suffix, param);
				if (!CO_OK(rc))
					return rc;
			}
		} while (exists);
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

static void split_comma_separated(const char *source, char **out_array, int *array_sizes, int array_length)
{
	int index = 0, j;
	
	for (index=0 ; index < array_length ; index++) {
		out_array[index][0] = '\0';
		j = 0;
		while (*source != ','  &&  *source != '\0') {
			if (j == array_sizes[index] - 1) {
				out_array[index][j] = '\0';
				break;
			}
			
			out_array[index][j] = *source;
			source++;
			j++;
		}
		
		if (*source == '\0')
			break;
		
		source++;
	}
}

static co_rc_t parse_args_networking_device_tap(co_config_t *conf, int index, const char *param)
{
	char host_ip[40] = {0, };
	char mac_address[40] = {0, };
	co_rc_t rc;

	char *array[3] = {
		conf->net_devs[index].desc,
		mac_address,
		host_ip,
	};
	int sizes[3] = {
		sizeof(conf->net_devs[index].desc),
		sizeof(mac_address),
		sizeof(host_ip),
	};

	split_comma_separated(param, array, sizes, 3);

	conf->net_devs[index].type = CO_NETDEV_TYPE_TAP;
	conf->net_devs[index].enabled = PTRUE;

	if (strlen(mac_address) > 0) {
		rc = co_parse_mac_address(mac_address, conf->net_devs[index].mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", mac_address);
			return rc;
		}
		conf->net_devs[index].manual_mac_address = PTRUE;
	}

	co_terminal_print("configured TAP device as eth%d\n", index);

	if (strlen(mac_address) > 0)
		co_terminal_print("MAC address: %s\n", mac_address);
	else
		co_terminal_print("MAC address: auto generated\n");

	if (strlen(host_ip) > 0)
		co_terminal_print("Host IP address: %s (currently ignored)\n", host_ip);

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_pcap(co_config_t *conf, int index, const char *param)
{
	char mac_address[40] = {0, };
	co_rc_t rc;

	char *array[3] = {
		conf->net_devs[index].desc,
		mac_address,
	};
	int sizes[3] = {
		sizeof(conf->net_devs[index].desc),
		sizeof(mac_address),
	};

	split_comma_separated(param, array, sizes, 3);

	conf->net_devs[index].type = CO_NETDEV_TYPE_BRIDGED_PCAP;
	conf->net_devs[index].enabled = PTRUE;

	if (strlen(mac_address) > 0) {
		rc = co_parse_mac_address(mac_address, conf->net_devs[index].mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", mac_address);
			return rc;
		}
		conf->net_devs[index].manual_mac_address = PTRUE;
	}

	if (strlen(conf->net_devs[index].desc) == 0) {
		co_terminal_print("error, the name of the network interface to attach was not specified\n");
		return CO_RC(ERROR);
	}

	co_terminal_print("configured PCAP bridge at '%s' device as eth%d\n", 
			  conf->net_devs[index].desc, index);

	if (strlen(mac_address) > 0)
		co_terminal_print("MAC address: %s\n", mac_address);
	else
		co_terminal_print("MAC address: auto generated\n");

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_slirp(co_config_t *conf, int index, const char *param)
{
	char mac_address[40] = {0, };

	char *array[3] = {
		conf->net_devs[index].desc,
		mac_address,
	};
	int sizes[3] = {
		sizeof(conf->net_devs[index].desc),
		sizeof(mac_address),
	};

	split_comma_separated(param, array, sizes, 3);

	conf->net_devs[index].type = CO_NETDEV_TYPE_SLIRP;
	conf->net_devs[index].enabled = PTRUE;

	co_terminal_print("configured Slirp as eth%d\n", index);

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
	char param[0x100];
	co_rc_t rc;

	do {
		int index;
		exists = PFALSE;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "eth", &index, param, 
							     sizeof(param), &exists);
		if (!CO_OK(rc)) 
			return rc;
		
		if (exists) {
			if (index < 0  || index >= CO_MODULE_MAX_CONET) {
				co_terminal_print("invalid network index: %d\n", index);
				return CO_RC(ERROR);
			}

			rc = parse_args_networking_device(conf, index, param);
			if (!CO_OK(rc))
				return rc;
		}
	} while (exists);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_cofs(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t exists;
	char param[0x100];
	co_rc_t rc;

	do {
		int index;
		exists = PFALSE;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "cofs", &index, param, 
							     sizeof(param), &exists);
		if (!CO_OK(rc)) 
			return rc;
		
		if (exists) {
			co_cofsdev_desc_t *cofs;

			if (index < 0  || index >= CO_MODULE_MAX_COFS) {
				co_terminal_print("invalid cofs index: %d\n", index);
				return CO_RC(ERROR);
			}

			cofs = &conf->cofs_devs[index];
			cofs->enabled = PTRUE;

			co_snprintf(cofs->pathname, sizeof(cofs->pathname), "%s", param);

			co_canonize_cobd_path(&cofs->pathname);
			
			co_terminal_print("mapping cofs%d to %s\n", index, cofs->pathname);
		}
	} while (exists);

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

	if (conf->initrd_enabled)
		co_terminal_print("using '%s' as initrd image\n", conf->initrd_path);

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

	return rc;
}

co_rc_t co_parse_config_args(co_command_line_params_t cmdline, co_start_parameters_t *start_parameters)
{
	co_rc_t rc, rc_;
	co_config_t *conf;

	start_parameters->cmdline_config = PFALSE;
	conf = &start_parameters->config;

	rc = co_cmdline_get_next_equality(cmdline, "kernel", 0, NULL, 0, 
					  conf->vmlinux_path, sizeof(conf->vmlinux_path),
					  &start_parameters->cmdline_config);

	if (!CO_OK(rc))
		return rc;

	if (!start_parameters->cmdline_config)
		return CO_RC(OK);

	co_terminal_print("using '%s' as kernel image\n", conf->vmlinux_path);

	rc = parse_config_args(cmdline, conf);
	
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
