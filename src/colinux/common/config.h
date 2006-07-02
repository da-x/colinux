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

#include "common.h"

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
	char alias[20];
} co_block_dev_desc_t;

typedef enum {
	CO_NETDEV_TYPE_BRIDGED_PCAP,
	CO_NETDEV_TYPE_TAP,
	CO_NETDEV_TYPE_SLIRP,
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
	bool_t manual_mac_address;
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
	char *desc;

	/*
	 * Individual mode setting
	 */
	char *mode;

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
	char *prog;

	/* Optional args */
	char *args;

	/* PID, if program running */
	int pid;

} co_execute_desc_t;

/*
 * Per-machine coLinux configuration
 */
typedef struct co_config {
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
	long block_root_device_index;

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
	 * The default size is 32MB for systems with more than
	 * 128MB of physical ram and 16MB for systems with less
	 * ram.
	 */ 
	unsigned long ram_size;

	/*
	 * The pathname of the initrd file.
	 */
	bool_t initrd_enabled;
	co_pathname_t initrd_path;
} co_config_t;

#endif
