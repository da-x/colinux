/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */
 /* OS independent command line and configuration file parser */

#include <string.h>

#include <colinux/common/libc.h>
#include <colinux/common/config.h>
#include <colinux/common/console.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/cobdpath.h>
#include <colinux/os/alloc.h>
#include "macaddress.h"

/* for types */
#include <scsi/coscsi.h>

#include "daemon.h"

#include <stdlib.h>

#define COLINUX_DEBUG_COCON	0
#define COLINUX_DEBUG_CURSOR	0
#define COLINUX_DEBUG_COLOR	0

typedef struct {
    int   size;
    char* buffer;
    } comma_buffer_t;	/* only for split_comma_separated here */

/* A bit mask of all used network types */
static int used_network_types;

/*
 * "New" command line configuration gathering scheme. It existed a long time in
 * User Mode Linux and should be less hussle than handling XML files for the
 * novice users.
 */

static int is_device(char* pathname)
{
        if (co_strncmp(pathname, "\\Device\\", 8) == 0  ||
            co_strncmp(pathname, "\\DosDevices\\", 12) == 0 ||
            co_strncmp(pathname, "\\Volume{", 8) == 0 ||
            co_strncmp(pathname, "\\\\.\\", 4) == 0)
		return 1;
	else
		return 0;
}

static co_rc_t check_cobd_file(co_pathname_t pathname, char* name, int index)
{
#ifdef COLINUX_DEBUG
	co_rc_t       rc;
	char*         buf;
	unsigned long size;

	static const char magic_bz2 [3] = "BZh";		/* bzip2 compressed data */
	static const char magic_7z  [6] = "7z\274\257\047\034";	/* 7z archive data */

	if (co_global_debug_levels.misc_level < 2)
		return CO_RC(OK);  /* verbose is not enabled */

	co_remove_quotation_marks(pathname);

	if (is_device(pathname)) return CO_RC(OK);  /* Can't check partitions or raw devices */

	rc = co_os_file_load(pathname, &buf, &size, 1024);
	if (!CO_OK(rc)) {
		co_terminal_print("%s%d: Error reading file (%s)\n", name, index, pathname);
		return rc;
	}

	if (size != 1024) {
		co_terminal_print("%s%d: File to small (%s)\n", name, index, pathname);
		rc = CO_RC(INVALID_PARAMETER);
	} else
	if (memcmp(buf, magic_bz2, sizeof(magic_bz2)) == 0 ||
	    memcmp(buf, magic_7z, sizeof(magic_7z)) == 0) {
		co_terminal_print("%s%d: Image file must be unpack before (%s)\n", name, index, pathname);
		rc = CO_RC(INVALID_PARAMETER);
	}

	co_os_file_free(buf);
	return rc;
#else
	return CO_RC(OK);  /* No debugging compiled in */
#endif
}

static co_rc_t parse_args_config_cobd(co_command_line_params_t cmdline, co_config_t* conf)
{
	bool_t	     exists;
	char*	     param;
	char	     buf[16];
	co_rc_t	     rc;
	unsigned int index;

	rc = co_cmdline_get_next_equality(cmdline, "setcobd", 0, NULL, 0,
					  buf, sizeof(buf), &exists);
	if (!CO_OK(rc))
		return rc;

	if (exists) {
		if (strcmp(buf, "async") == 0) {
			conf->cobd_async_enable = PTRUE;
		} else if (strcmp(buf, "sync") == 0) {
			conf->cobd_async_enable = PFALSE;
		} else {
			co_terminal_print("error: setcobd option only allowed"
			                  " 'async' or 'sync'\n");
			return CO_RC(INVALID_PARAMETER);
		}
	}

	do {
		co_block_dev_desc_t *cobd;

		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							     "cobd",
							     &index,
							     CO_MODULE_MAX_COBD,
							     &param,
							     &exists);
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

		rc = check_cobd_file(cobd->pathname, "cobd", index);
		if (!CO_OK(rc))
			return rc;

		co_canonize_cobd_path(&cobd->pathname);
		co_debug_info("mapping cobd%d to %s", index, cobd->pathname);

	} while (1);

	return CO_RC(OK);
}

static co_rc_t allocate_by_alias(co_config_t* conf,
                                 const char*  prefix,
                                 const char*  suffix,
				 const char*  param)
{
	co_block_dev_desc_t* cobd;
	int		     i;
	co_rc_t		     rc;

	for (i = 0; i < CO_MODULE_MAX_COBD; i++) {
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

	rc = check_cobd_file (cobd->pathname, "cobd", i);
	if (!CO_OK(rc))
		return rc;

	co_canonize_cobd_path(&cobd->pathname);

	cobd->alias_used = PTRUE;
	co_snprintf(cobd->alias, sizeof(cobd->alias), "%s%s", prefix, suffix);

	co_debug_info("selected cobd%d for %s, mapping to '%s'", i, cobd->alias, cobd->pathname);

	return CO_RC(OK);
}

static co_rc_t parse_args_config_aliases(co_command_line_params_t cmdline, co_config_t* conf)
{
	const char* prefixes[] = {"sd", "hd"};
	const char* prefix;
	bool_t      exists;
	char*	    param;
	co_rc_t     rc;
	int 	    i;

	for (i = 0; i < sizeof(prefixes) / sizeof(char *); i++) {
		prefix = prefixes[i];
		char suffix[5];

		do {
			rc = co_cmdline_get_next_equality_alloc(cmdline,
								prefix,
								sizeof(suffix) - 1,
								suffix,
								sizeof(suffix),
							  	&param,
							  	&exists);
			if (!CO_OK(rc))
				return rc;

			if (!exists)
				break;

 			if (!co_strncmp(":cobd", param, 5)) {
				char*                index_str    = &param[5];
				char* 	             number_parse = NULL;
				unsigned int 	     index;
				co_block_dev_desc_t* cobd;

				index = strtoul(index_str, &number_parse, 10);
				if (number_parse == index_str) {
					co_terminal_print("invalid alias: %s%s=%s\n", prefix, suffix, param);
					return CO_RC(INVALID_PARAMETER);
				}

				if (index >= CO_MODULE_MAX_COBD) {
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

static bool_t strmatch_identifier(const char* str, const char* identifier, const char** end)
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

static void split_comma_separated(const char* source, comma_buffer_t* array)
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

static co_rc_t parse_args_config_pci(co_command_line_params_t cmdline, co_config_t* conf)
{
	char 	func[8];
	char 	type[16];
	char 	unit[8];
	int	d,f,t,u;
	bool_t 	exists;
	char*	param;

	comma_buffer_t array [] = {
		{ sizeof(func), func },
		{ sizeof(type), type },
		{ sizeof(unit), unit },
		{ 0, NULL }
	};
	co_rc_t      rc;
	unsigned int index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							     "pci",
							     &index,
							     32,
							     &param,
							     &exists);
		if (!CO_OK(rc)) return rc;
		if (!exists) break;

		split_comma_separated(param, array);

		if (index < 0 || index > 31) {
			co_terminal_print("pci: device: %d out of range (0-31)!", index);
			return CO_RC(INVALID_PARAMETER);
		}
		d = index;
		f = atoi(func);
		if (f < 0 || f > 7) {
			co_terminal_print("pci%d: function: %s out of range (0-7)\n", index, func);
			return CO_RC(INVALID_PARAMETER);
		}
		u = atoi(unit);
#ifdef CONFIG_COOPERATIVE_VIDEO
		if (strcmp(type,"video") == 0) {
			t = CO_DEVICE_VIDEO;
			if (u < 0 || u >= CO_MODULE_MAX_COVIDEO) {
				co_terminal_print("pci%d: unit: %s out of range for a video device (0-%d)\n", index, unit,
					CO_MODULE_MAX_COVIDEO-1);
				return CO_RC(INVALID_PARAMETER);
			}
			if (!conf->video_devs[u].enabled) {
				co_terminal_print("pci%d: video unit %d is not enabled.\n", index, u);
				return CO_RC(INVALID_PARAMETER);
			}
		} else
#endif
#ifdef CONFIG_COOPERATIVE_AUDIO
		if (strcmp(type,"audio") == 0) {
			t = CO_DEVICE_AUDIO;
			if (u < 0 || u >= CO_MODULE_MAX_COAUDIO) {
				co_terminal_print("pci%d: unit: %s out of range for a audio device (0-%d)\n", index, unit,
					CO_MODULE_MAX_COAUDIO-1);
				return CO_RC(INVALID_PARAMETER);
			}
			if (!conf->audio_devs[u].enabled) {
				co_terminal_print("pci%d: audio unit %d is not enabled.\n", index, u);
				return CO_RC(INVALID_PARAMETER);
			}
		} else
#endif
#ifdef CO_DEVICE_IDE
		if (strcmp(type,"ide") == 0) {
			int x,found;

			t = CO_DEVICE_IDE;
			found = 0;
			for(x=0; x < CO_MODULE_MAX_COIDE; x++) {
				if (conf->ide_devs[x].enabled) {
					found = 1;
					break;
				}
			}
			if (!found) {
				co_terminal_print("pci%d: no ide devices are enabled.\n", index);
				return CO_RC(INVALID_PARAMETER);
			}
		} else
#endif
		if (strcmp(type,"scsi") == 0) {
			int x,found;

			t = CO_DEVICE_SCSI;
			found = 0;
			for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
				if (conf->scsi_devs[x].enabled) {
					found = 1;
					break;
				}
			}
			if (!found) {
				co_terminal_print("pci%d: no scsi devices are enabled.\n", index);
				return CO_RC(INVALID_PARAMETER);
			}
		}
		else if (strcmp(type,"network") == 0 || strcmp(type,"net") == 0) {
			t = CO_DEVICE_NETWORK;
			if (u < 0 || u >= CO_MODULE_MAX_CONET) {
				co_terminal_print("pci%d: unit: %s out of range for a network device (0-%d)\n", index, unit,
					CO_MODULE_MAX_CONET-1);
				return CO_RC(INVALID_PARAMETER);
			}
			if (!conf->net_devs[u].enabled) {
				co_terminal_print("pci%d: network unit %d is not enabled.\n", index, u);
				return CO_RC(INVALID_PARAMETER);
			}
		} else {
			co_terminal_print("pci%d: unknown device type: %s\n", index, type);
			return CO_RC(INVALID_PARAMETER);
		}

		co_debug_info("pci%d: func: %d, type: %d, unit: %d", d, f, t, u);

		conf->pci[d][f].type = t;
		conf->pci[d][f].unit = u;
	} while (1);

	return CO_RC(OK);
}

#ifdef CONFIG_COOPERATIVE_VIDEO
static co_rc_t parse_args_config_video(co_command_line_params_t cmdline, co_config_t *conf)
{
	char size[16];
	co_video_dev_desc_t *video;
	bool_t exists;
	char *param;
	comma_buffer_t array [] = {
		{ sizeof(size), size },
		{ 0, NULL }
	};
	co_rc_t rc;
	unsigned int index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline, "video",
							     &index, CO_MODULE_MAX_COVIDEO,
							     &param, &exists);
		if (!CO_OK(rc)) return rc;
		if (!exists) break;

		video = &conf->video_devs[index];
		if (video->enabled) {
			co_terminal_print("video%d double defined\n", index);
			return CO_RC(INVALID_PARAMETER);
		}

		split_comma_separated(param, array);

		/* Video size must be non-zero */
		video->size = atoi(size);
		if (video->size < 1 || video->size > 16) {
			co_terminal_print("video%d: invalid size (%d)\n", index, video->size);
			return CO_RC(INVALID_PARAMETER);
		}
		video->size *= (1024 * 1024);

		co_debug_info("video%d: size: %dK", index, video->size >> 10);

		video->enabled = PTRUE;
	} while (1);

	return CO_RC(OK);
}
#endif

static co_rc_t parse_args_config_scsi(co_command_line_params_t cmdline, co_config_t* conf)
{
	char type[10], path[256], size[16], mode[10];
	co_scsi_dev_desc_t *scsi;
	bool_t exists;
	char *param;
	/* Type, Path, Size */
	comma_buffer_t array [] = {
		{ sizeof(type), type },
		{ sizeof(path), path },
		{ sizeof(size), size },
		{ sizeof(mode), mode },
		{ 0, NULL }
	};
	co_rc_t rc;
	unsigned int index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							     "scsi",
							     &index,
							     CO_MODULE_MAX_COSCSI,
							     &param,
							     &exists);
		if (!CO_OK(rc)) return rc;
		if (!exists) break;

		scsi = &conf->scsi_devs[index];

		if (scsi->enabled) {
			co_terminal_print("scsi%d double defined\n", index);
			return CO_RC(INVALID_PARAMETER);
		}
		scsi->enabled = PTRUE;

		split_comma_separated(param, array);
		if (strcmp(type,"pass") == 0)
			scsi->type = SCSI_PTYPE_PASS;
		else if (strcmp(type,"disk") == 0)
			scsi->type = SCSI_PTYPE_DISK;
		else if (strcmp(type,"cdrom") == 0)
			scsi->type = SCSI_PTYPE_CDDVD;
		else if (strcmp(type,"cdrw") == 0)
			scsi->type = SCSI_PTYPE_WORM;
		else if (strcmp(type,"tape") == 0)
			scsi->type = SCSI_PTYPE_TAPE;
		else if (strcmp(type,"changer") == 0)
			scsi->type = SCSI_PTYPE_CHANGER;
		else {
			co_terminal_print("scsi%d: unknown type: %s\n", index, type);
			return CO_RC(INVALID_PARAMETER);
		}
		strcpy(scsi->pathname, path);
		scsi->is_dev = is_device(path);
		scsi->size = atoi(size);
		scsi->shared = (strcmp(mode,"shared") == 0);

		switch(scsi->type) {
		case SCSI_PTYPE_PASS:
			co_debug_info("scsi%d: pass-through to %s", index, scsi->pathname);
			break;
		case SCSI_PTYPE_CHANGER:
			co_debug_info("scsi%d: %d slot autochanger for devices %s", index, scsi->size, scsi->pathname);
			break;
		default:
			if (!scsi->size) {
				rc = check_cobd_file(scsi->pathname, "scsi", index);
				if (!CO_OK(rc)) return rc;
			} else if (scsi->size < 0 || scsi->size > 0x100000) {
				co_terminal_print("scsi%d: Size '%s' out of range. (max. 1048576 MB)\n", index, size);
				return CO_RC(INVALID_PARAMETER);
			}

			co_canonize_cobd_path(&scsi->pathname);
			co_debug_info("scsi%d: %s -> %s (size: %d MB)", index, type, scsi->pathname, scsi->size);
			break;
		}
	} while (1);

	return CO_RC(OK);
}

static co_rc_t config_parse_mac_address(const char* text, co_netdev_desc_t* net_dev)
{
	co_rc_t rc;

	if (*text) {
		rc = co_parse_mac_address(text, net_dev->mac_address);
		if (!CO_OK(rc)) {
			co_terminal_print("error parsing MAC address: %s\n", text);
			return rc;
		}
		net_dev->manual_mac_address = PTRUE;

		co_debug_info("MAC address: %s", text);
	} else {
		co_debug("MAC address: auto generated");
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_tap(co_config_t* conf, int index, const char* param)
{
	co_netdev_desc_t* net_dev = &conf->net_devs[index];
	char 		  mac_address[40];
	char 		  host_ip[40];
	co_rc_t 	  rc;

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

	used_network_types |= 1<<CO_NETDEV_TYPE_TAP;
	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_pcap(co_config_t* conf, int index, const char* param)
{
	co_netdev_desc_t* net_dev = &conf->net_devs[index];
	char		  mac_address[40];
	char		  promisc_mode[10];
	co_rc_t		  rc;

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

	used_network_types |= 1<<CO_NETDEV_TYPE_BRIDGED_PCAP;
	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_slirp(co_config_t *conf, int index, const char *param)
{
	co_netdev_desc_t* net_dev = &conf->net_devs[index];
	char		  mac_address[40];
	co_rc_t		  rc;

	comma_buffer_t array [] = {
		{ sizeof(mac_address), mac_address },
		{ sizeof(net_dev->redir), net_dev->redir },
		{ 0, NULL }
	};

	split_comma_separated(param, array);

	net_dev->type    = CO_NETDEV_TYPE_SLIRP;
	net_dev->enabled = PTRUE;

	co_debug_info("configured Slirp as eth%d", index);

	rc = config_parse_mac_address(mac_address, net_dev);
	if (!CO_OK(rc))
		return rc;

	if (*net_dev->redir)
		co_debug_info("redirections %s", net_dev->redir);

	used_network_types |= 1 << CO_NETDEV_TYPE_SLIRP;
	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device_ndis(co_config_t *conf, int index, const char *param)
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

	net_dev->type = CO_NETDEV_TYPE_NDIS_BRIDGE;
	net_dev->enabled = PTRUE;
	net_dev->promisc_mode = 1;

	co_debug_info("configured NDIS bridge at '%s' device as eth%d",
			net_dev->desc, index);

	rc = config_parse_mac_address(mac_address, net_dev);
	if (!CO_OK(rc))
		return rc;

	if (*promisc_mode) {
		if (strcmp(promisc_mode, "nopromisc") == 0) {
			net_dev->promisc_mode = 0;
			co_debug_info("Ndis mode: nopromisc");
		} else if (strcmp(promisc_mode, "promisc") == 0) {
			net_dev->promisc_mode = 1;
			co_debug_info("Ndis mode: promisc");
		} else {
			co_terminal_print("error: Ndis bridge option only allowed 'promisc' or 'nopromisc'\n");
			return CO_RC(INVALID_PARAMETER);
		}
	}

	used_network_types |= 1<<CO_NETDEV_TYPE_NDIS_BRIDGE;
	return CO_RC(OK);
}

static co_rc_t parse_args_networking_device(co_config_t *conf, int index, const char *param)
{
	const char* next = NULL;

	if (strmatch_identifier(param, "tuntap", &next)) {
		return parse_args_networking_device_tap(conf, index, next);
	} else if (strmatch_identifier(param, "pcap-bridge", &next)) {
		return parse_args_networking_device_pcap(conf, index, next);
	} else if (strmatch_identifier(param, "slirp", &next)) {
		return parse_args_networking_device_slirp(conf, index, next);
	} else if (strmatch_identifier(param, "ndis-bridge", &next)) {
		return parse_args_networking_device_ndis(conf, index, next);
	} else {
		co_terminal_print("unsupported network transport type: %s\n", param);
		co_terminal_print("supported types are: tuntap, pcap-bridge, ndis-bridge, slirp\n");
		return CO_RC(INVALID_PARAMETER);
	}

	return CO_RC(OK);
}

static co_rc_t parse_args_networking(co_command_line_params_t cmdline, co_config_t *conf)
{
	bool_t	     exists;
	char*	     param;
	co_rc_t	     rc;
	unsigned int index;

	do {
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

static co_rc_t parse_args_cofs_device(co_config_t* conf, int index, const char* param)
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

static co_rc_t parse_args_config_cofs(co_command_line_params_t cmdline, co_config_t* conf)
{
	bool_t       exists;
	char*	     param;
	co_rc_t      rc;
	unsigned int index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							     "cofs",
							     &index,
							     CO_MODULE_MAX_COFS,
							     &param,
							     &exists);
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

static co_rc_t parse_args_serial_device(co_config_t* conf, int index, const char* param)
{
	co_serialdev_desc_t* serial = &conf->serial_devs[index];
	char 		     name [CO_SERIAL_DESC_STR_SIZE];
	char 		     mode [CO_SERIAL_MODE_STR_SIZE];

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

static co_rc_t parse_args_config_serial(co_command_line_params_t cmdline, co_config_t* conf)
{
	bool_t 		exists;
	char*		param;
	co_rc_t 	rc;
	unsigned int 	index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							    "ttys",
							     &index,
							     CO_MODULE_MAX_SERIAL,
							     &param,
							     &exists);
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

static co_rc_t parse_args_execute(co_config_t* conf, int index, const char* param)
{
	co_execute_desc_t* execute = &conf->executes[index];
	char		   prog [CO_EXECUTE_PROG_STR_SIZE];
	char		   args [CO_EXECUTE_ARGS_STR_SIZE];

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

static co_rc_t parse_args_config_execute(co_command_line_params_t cmdline, co_config_t* conf)
{
	bool_t	     exists;
	char*	     param;
	co_rc_t      rc;
	unsigned int index;

	do {
		rc = co_cmdline_get_next_equality_int_prefix(cmdline,
							     "exec",
							     &index,
							     CO_MODULE_MAX_EXECUTE,
							     &param,
							     &exists);
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

static co_rc_t parse_args_config_cocon(co_command_line_params_t cmdline, co_config_t* conf)
{
	bool_t 	exists;
	char 	buf[16];
	char*	p;
	co_rc_t rc;

	conf->console.x     = CO_CONSOLE_WIDTH;
	conf->console.y     = CO_CONSOLE_HEIGHT;
	conf->console.max_y = CO_CONSOLE_MAX_ROWS;

	rc = co_cmdline_get_next_equality(cmdline,
					  "cocon",
					  0,
					  NULL,
					  0,
					  buf,
					  sizeof(buf),
					  &exists);
	if (!CO_OK(rc))
		return rc;

	if (exists) {
		int x;
		int y;
		int max_y;

		x = strtol(buf, &p, 0);
		if (*p != '\0') p++;
		y = strtol(p, &p, 0);
		if (*p != '\0') p++;
		max_y = strtol(p, &p, 0);

		if(!max_y) max_y = CO_CONSOLE_MAX_ROWS;
		if(max_y < y) max_y = y;

		/* Check screen limits */
		if(x     < CO_CONSOLE_MIN_COLS	||
		   y     < CO_CONSOLE_MIN_ROWS  ||
		   max_y > CO_CONSOLE_MAX_ROWS) {
			co_terminal_print("Invalid args (%ux%ux%u) for cocon\n",
					  x, y, max_y);
			return CO_RC(INVALID_PARAMETER);
		}
		conf->console.x     = x;
		conf->console.y     = y;
		conf->console.max_y = max_y;
	}
#if COLINUX_DEBUG_COCON
	co_terminal_print("cocon=%dx%d,%d\n",
			  (int)conf->console.x,
			  (int)conf->console.y,
			  (int)conf->console.max_y);
#endif
	return CO_RC(OK);
}

/* Set console cursor size */
static co_rc_t parse_args_config_cursor(co_command_line_params_t cmdline,
                                        co_config_t*             conf)
{
	bool_t  exists;
	char    buf[16];
	char*   p;
	co_rc_t rc;

	conf->console.curs_type_size = CO_CUR_DEFAULT;

	rc = co_cmdline_get_next_equality(cmdline,
					  "cursor",
					  0,
					  NULL,
					  0,
					  buf,
					  sizeof(buf),
					  &exists);
	if (!CO_OK(rc))
		return rc;

	if (exists) {
		int size_prc, cursize;

		size_prc = (int)strtol(buf, &p, 0);

		if (size_prc < 0 || size_prc > 100) {
			co_terminal_print("Invalid arg (%d) for cursor\n", size_prc);
			return CO_RC(INVALID_PARAMETER);
		}

		/* User can set size of cursor == 100, but actual value of
		 * it can not exceed 99. Set cursor size in accordance with
		 * the kernel.
		 */
		if      (size_prc <=   0) cursize = CO_CUR_NONE;
		else if (size_prc <=  16) cursize = CO_CUR_UNDERLINE;
		else if (size_prc <=  33) cursize = CO_CUR_LOWER_THIRD;
		else if (size_prc <=  50) cursize = CO_CUR_LOWER_HALF;
		else if (size_prc <=  66) cursize = CO_CUR_TWO_THIRDS;
		else if (size_prc <= 100) cursize = CO_CUR_BLOCK;
		else			  cursize = CO_CUR_DEFAULT;

		conf->console.curs_type_size = cursize;
	}
#if COLINUX_DEBUG_CURSOR
	co_terminal_print("cursor=%d\n", conf->console.curs_type_size);
#endif
	return CO_RC(OK);
}

#if CO_ENABLE_CON_COLOR

typedef struct co_color_tbl_t
{ char*	name_p;
  int	len;
  int	attr;
}co_color_tbl_t;

static co_color_tbl_t const co_color_tbl[] =
{ /* Dark colors */
  {"black"        , 5, CO_COLOR_BLACK              	},
  {"red"          , 3, CO_COLOR_RED                	},
  {"green"        , 5, CO_COLOR_GREEN              	},
  {"brown"        , 5, CO_COLOR_BROWN             	},
  {"blue"         , 4, CO_COLOR_BLUE			},
  {"magenta"      , 7, CO_COLOR_MAGENTA			},
  {"cyan"         , 4, CO_COLOR_CYAN			},
  {"gray"     	  , 4, CO_COLOR_GRAY			},
  /* Bright colors */
  {"darkgray"     , 8, CO_COLOR_BLACK   | CO_ATTR_BRIGHT}, /* Or "brightblack" :) */
  {"brightred"    , 9, CO_COLOR_RED     | CO_ATTR_BRIGHT},
  {"brightgreen"  ,11, CO_COLOR_GREEN   | CO_ATTR_BRIGHT},
  {"yellow"       , 6, CO_COLOR_BROWN   | CO_ATTR_BRIGHT},
  {"brightblue"   ,10, CO_COLOR_BLUE    | CO_ATTR_BRIGHT},
  {"brightmagenta",13, CO_COLOR_MAGENTA | CO_ATTR_BRIGHT},
  {"brightcyan"   ,10, CO_COLOR_CYAN    | CO_ATTR_BRIGHT},
  {"white"        , 5, CO_COLOR_GRAY    | CO_ATTR_BRIGHT},
  {NULL		  , 0, 0x00				}
};

static int co_find_color_attr(char** str_pp)
{
	char*	beg_p;
	char*	end_p;
	int	len;


	beg_p = *str_pp;
	end_p = beg_p;

	/* Find word end */
	while(*end_p != ',' && *end_p != '\0') {
		end_p++;
	}

	len = (int)(end_p - beg_p);
	if(len > 0) {
		co_color_tbl_t* tbl_p;

		tbl_p = (co_color_tbl_t*)co_color_tbl;
		while(tbl_p->name_p != NULL) {
			if(len == tbl_p->len &&
		   	strncmp(tbl_p->name_p, beg_p, len) == 0) {
		   		*str_pp = end_p;
		   		return tbl_p->attr;
			}
			tbl_p++;
		}
	}
	return -1;
}

#endif /* CO_ENABLE_CON_COLOR */

/* Set console screen color */
static co_rc_t parse_args_config_color(co_command_line_params_t cmdline,
                                       co_config_t*             conf)
{
#if CO_ENABLE_CON_COLOR
	bool_t  exists;
	char    buf[32];
	co_rc_t rc;
#endif

	conf->console.attr = CO_ATTR_DEFAULT;

#if CO_ENABLE_CON_COLOR
	rc = co_cmdline_get_next_equality(cmdline,
					  "color",
					  0,
					  NULL,
					  0,
					  buf,
					  sizeof(buf),
					  &exists);
	if (exists) {
		char* str_p;
		int   fg_attr;
		int   bg_attr;

		str_p = buf;

		fg_attr = co_find_color_attr(&str_p);
		if(*str_p != '\0') str_p++;
		bg_attr = co_find_color_attr(&str_p);

		if(fg_attr < 0 || bg_attr < 0) {
			co_terminal_print("Invalid arg (%s) for color\n", buf);
			return CO_RC(INVALID_PARAMETER);
		} else if(fg_attr == bg_attr) {
			co_terminal_print("Warning, "
					  "foreground == background: %s\n", buf);
			return CO_RC(INVALID_PARAMETER);
		}
		conf->console.attr = (bg_attr << 4) | fg_attr;
	}
#endif /* CO_ENABLE_CON_COLOR */

#if COLINUX_DEBUG_COLOR
	co_terminal_print("color=%02x,%02x\n",
			  conf->console.attr & 0x0F,
			  conf->console.attr >> 4);
#endif
	return CO_RC(OK);
}

/* Parse config file specific parameters */
static co_rc_t parse_config_args(co_command_line_params_t cmdline, co_config_t* conf)
{
	co_rc_t rc;
	bool_t  exists;

	rc = co_cmdline_get_next_equality_int_value(cmdline,
						    "mem",
						    &conf->ram_size,
						    &exists);
	if (!CO_OK(rc))
		return rc;

	if (exists)
		co_debug_info("configuring %u MB of virtual RAM", conf->ram_size);

#ifdef CONFIG_COOPERATIVE_VIDEO
	rc = parse_args_config_video(cmdline, conf);
	if (!CO_OK(rc))
		return rc;
#endif

	rc = parse_args_config_scsi(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

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

	/* Must be last (after all other devs are defined) */
	rc = parse_args_config_pci(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_get_next_equality(cmdline,
					  "initrd",
					  0,
					  NULL,
					  0,
					  conf->initrd_path,
					  sizeof(conf->initrd_path),
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
				rc = parse_args_cofs_device(conf, CO_MODULE_MAX_COFS - 1, param);

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

	rc = parse_args_config_color(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_config_cocon(cmdline, conf);
	if (!CO_OK(rc))
		return rc;

	rc = parse_args_config_cursor(cmdline, conf);
	if (!CO_OK(rc))
		return rc;
	return rc;
}

co_rc_t co_parse_config_args(co_command_line_params_t cmdline,
       			     co_start_parameters_t*   start_parameters)
{
	co_rc_t      rc;
	co_rc_t      rc_;
	co_config_t* conf = &start_parameters->config;

	start_parameters->cmdline_config = PFALSE;

	rc = co_cmdline_get_next_equality(cmdline,
				          "kernel",
				          0,
				          NULL,
				          0,
					  conf->vmlinux_path,
					  sizeof(conf->vmlinux_path),
					  &start_parameters->cmdline_config);
	if (!CO_OK(rc))
		return rc;

	if (!start_parameters->cmdline_config)
		return CO_RC(OK);

	co_remove_quotation_marks(conf->vmlinux_path);
	co_debug_info("using '%s' as kernel image", conf->vmlinux_path);

	rc = parse_config_args(cmdline, conf);

	rc_ = co_cmdline_params_format_remaining_parameters(cmdline,
							    conf->boot_parameters_line,
							    sizeof(conf->boot_parameters_line));
	if (!CO_OK(rc_))
		return rc_;

	if (CO_OK(rc)) {
		co_debug_info("kernel boot parameters: '%s'", conf->boot_parameters_line);
		start_parameters->config_specified = PTRUE;
	}

	start_parameters->network_types |= used_network_types;

	return rc;
}
