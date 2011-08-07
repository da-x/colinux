/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_CONFIG_H__
#define __COLINUX_LINUX_CONFIG_H__

#if defined __cplusplus
extern "C" {
#endif

#include "common.h"

#define CO_ENABLE_CON_SCROLL	0 /* 1=Enable console scrolling,    0=disable */
#define CO_ENABLE_CON_COLOR	1 /* 1=Enable console screen color, 0=disable */

/* Color attributes */
typedef enum co_color_t
{ CO_COLOR_BLACK   = 0x00,
  CO_COLOR_BLUE    = 0x01,
  CO_COLOR_GREEN   = 0x02,
  CO_COLOR_RED     = 0x04,
  CO_COLOR_BROWN   = CO_COLOR_RED   | CO_COLOR_GREEN,
  CO_COLOR_MAGENTA = CO_COLOR_RED   | CO_COLOR_BLUE,
  CO_COLOR_CYAN    = CO_COLOR_GREEN | CO_COLOR_BLUE,
  CO_COLOR_GRAY    = CO_COLOR_RED   | CO_COLOR_GREEN | CO_COLOR_BLUE
}co_color_t;

#define CO_ATTR_BRIGHT		0x08
#define CO_ATTR_DEFAULT		((CO_COLOR_BLACK << 4) | CO_COLOR_GRAY)

/*
 * Per block device configuration
 */
typedef struct co_block_dev_desc {
	/*
	 * This bool var determines whether Linux would be given
	 * permission to access this device.
	 */
	bool_t enabled;

	/*
	 * The pathname of the host OS file (or device) that represents
	 * this block device. Can be relative to the pathname of the
	 * configration file.
	 */
	co_pathname_t pathname;

	/*
	 * The alias is a synonym device name for the block device on
	 * the Linux side, for example: hda4, sda2, hdb2.
	 */
	bool_t alias_used;
	char   alias[20];
} co_block_dev_desc_t;
typedef struct co_video_desc {
        unsigned int    size;
        unsigned short  width;
        unsigned short  height;
        unsigned char   bpp;
} co_video_desc_t;

typedef struct co_video_dev_desc {
	bool_t 	enabled;
        co_video_desc_t desc;
} co_video_dev_desc_t;

typedef struct co_audio_dev_desc {
	bool_t	enabled;
} co_audio_dev_desc_t;

typedef struct co_scsi_dev_desc {
	bool_t	enabled;
	int	type;				/* SCSI type */
	co_pathname_t pathname;			/* Path */
	int	is_dev;				/* 0 = file, 1 = device */
	int	size;				/* Size, for files */
	int	shared;				/* 0 = exclusive, 1 = shared access */
} co_scsi_dev_desc_t;

typedef enum {
	CO_NETDEV_TYPE_BRIDGED_PCAP,
	CO_NETDEV_TYPE_TAP,
	CO_NETDEV_TYPE_SLIRP,
	CO_NETDEV_TYPE_NDIS_BRIDGE,		/* kernel mode conet bridge */
	CO_NETDEV_TYPE_NDIS_NAT,		/* kernel mode conet NAT */
	CO_NETDEV_TYPE_NDIS_HOST		/* kernel mode conet HOST only network */
} co_netdev_type_t;

#define CO_NETDEV_DESC_STR_SIZE 0x40
#define CO_NETDEV_REDIRDIR_STR_SIZE 0xFF

/*
 * Per network device configuration
 */
typedef struct co_netdev_desc {
	/*
	 * Determines whether we use this device slot or not.
	 */
	bool_t enabled;

	/* Used for indentifing the interface on the host side */
	char desc[CO_NETDEV_DESC_STR_SIZE];

	/*
	 * Type of device:
	 *
	 * Bridged pcap - 'desc' is the name of the network interface to
         *                 bridge into
	 * TAP - 'desc' is the name of the TAP network interface (Win32 TAP
	 *       on Windows).
	 */
	co_netdev_type_t type;

	/*
	 * MAC address for the Linux side.
	 */
	bool_t 	      manual_mac_address;
	unsigned char mac_address[6];

	/* Slirp Parameters */
	char redir[CO_NETDEV_REDIRDIR_STR_SIZE];

	/* Bridged Parameters */
	/* http://www.winpcap.org/docs/docs31/html/group__wpcapfunc.html#ga1 */
	/* promiscuous mode (nonzero means promiscuous) */
	int promisc_mode;
} co_netdev_desc_t;

typedef enum {
	/*
	 * Flat mode - we use as much meta data as we can from the hot
	 * os in order to provide a UNIX file system. It's a simple
	 * effortless implementation of an host FS.
	 */
	CO_COFS_TYPE_FLAT = 1,

	/*
	 * Unix Meta-data - we maintain the UNIX metadata of the guest
	 * mount in a parallel host FS directory structure. This means
	 * that the guest file meta data is disconnected from the host
	 * OS file meta data entirely, allowing to provide a 'true' UNIX
	 * file system on Windows, for example.
	 */
	CO_COFS_TYPE_UNIX_METADATA = 2,
} co_cofs_type_t;

typedef struct co_cofsdev_desc_t {
	/*
	 * Determines whether we use this device slot or not.
	 */
	bool_t enabled;

	/* Fully qualified kernel pathname for the mounted file system */
	co_pathname_t pathname;

	/* Host-OS type of mount */
	co_cofs_type_t type;
} co_cofsdev_desc_t;

#define CO_SERIAL_DESC_STR_SIZE 0x40
#define CO_SERIAL_MODE_STR_SIZE 0x100

typedef struct co_serialdev_desc {
	/*
	 * Determines whether we use this device slot or not.
	 */
	bool_t enabled;

	/* Device name on host side */
	char* desc;

	/*
	 * Individual mode setting
	 */
	char* mode;

} co_serialdev_desc_t;

#define CO_MODULE_MAX_EXECUTE 8
#define CO_EXECUTE_PROG_STR_SIZE 0x100
#define CO_EXECUTE_ARGS_STR_SIZE 0x200

typedef struct co_execute_desc {
	/*
	 * Determines whether we use this device slot or not.
	 */
	bool_t enabled;

	/* Executable program name */
	char* prog;

	/* Optional args */
	char* args;

	/* PID, if program running */
	int pid;

} co_execute_desc_t;

/* User defined console configuration */
typedef struct co_config_console {
	/* Dimensions of the console screen */
	int x;
	int y;

	/* Dimensions of the console buffer */
	/* int max_x; */
	int max_y;

	/* Cursor size in Linux kernel types */
	int curs_type_size;

	/* Colors for clear screen: (bg << 4) | fg */
	int attr;
} co_console_config_t;

/*
 * Per-machine coLinux configuration
 */
typedef struct co_config {

	/*
	 * Size of this struct. Avoids crash on future changes.
	 * This struct is shared between userland daemon and kernel driver.
	 */
	int magic_size;

	/*
	 * Absolute pathname of the configuration file (relative pathnames
	 * will be looked upon according to this path).
	 */
	co_pathname_t config_path;

	/*
	 * The pathname of the vmlinux file. If this is empty then we
	 * would try by default to locate 'vmlinux' in the same directory
	 * where the configuration file resides.
	 */
	co_pathname_t vmlinux_path;

	/*
	 * Information about block devices that we import to
	 * Linux and the index of the block device to boot from.
	 */
	co_block_dev_desc_t block_devs[CO_MODULE_MAX_COBD];

	/*
	 * PCI config (32 devs, 8 funcs/dev)
	 */
	struct {
		unsigned char type;
		unsigned char unit;
	} pci[32][8];

	/*
	 * Video devices
	 */
	co_video_dev_desc_t video_devs[CO_MODULE_MAX_COVIDEO];

	/*
	 * SCSI devices
	 */
	co_scsi_dev_desc_t scsi_devs[CO_MODULE_MAX_COSCSI];

	/*
	 * Network devices
	 */
	co_netdev_desc_t net_devs[CO_MODULE_MAX_CONET];

	/*
	 * File systems
	 */
	co_cofsdev_desc_t cofs_devs[CO_MODULE_MAX_COFS];

	/*
	 * Serial devices
	 */
	co_serialdev_desc_t serial_devs[CO_MODULE_MAX_SERIAL];

	/*
	 * Audio devices
	 */
	co_audio_dev_desc_t audio_devs[CO_MODULE_MAX_COAUDIO];

	/*
	 * Executable programs
	 */
	co_execute_desc_t executes[CO_MODULE_MAX_EXECUTE];

	/*
	 * Parameters passed to the kernel at boot.
	 */
	char boot_parameters_line[CO_BOOTPARAM_STRING_LENGTH];

	/*
	 * Size of pseudo physical RAM for this machine (MB).
	 *
	 * The default size is 25% of total RAM for systems with more than
	 * 128MB of physical ram and 16MB for systems with less ram.
	 */
	unsigned int ram_size;

	/*
	 * The pathname of the initrd file.
	 */
	bool_t		initrd_enabled;
	co_pathname_t	initrd_path;

	/*
	 * Enable asynchronious block device operations.
	 */
	int cobd_async_enable;

	/* User console configuration */
	co_console_config_t console;

} co_config_t;

#if defined __cplusplus
}
#endif

#endif /* __COLINUX_LINUX_CONFIG_H__ */
