/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_COMMON_IOCTL_H__
#define __COLINUX_COMMON_IOCTL_H__

#include "common.h"
#include "keyboard.h"

#include <colinux/common/import.h>
#include <colinux/common/config.h>

typedef enum {
	CO_MANAGER_IOCTL_BASE=0x10,

	CO_MANAGER_IOCTL_INIT,
	CO_MANAGER_IOCTL_CREATE,
	CO_MANAGER_IOCTL_MONITOR,
	CO_MANAGER_IOCTL_STATUS,
} co_manager_ioctl_t;

/* interface for CO_MANAGER_IOCTL_INIT: */
typedef struct {
	unsigned long physical_memory_size;
} co_manager_ioctl_init_t;

/* interface for CO_MANAGER_IOCTL_CREATE: */
typedef struct {
	co_rc_t rc;
	co_id_t new_id;
	co_symbols_import_t import;
	co_config_t config;
} co_manager_ioctl_create_t;

/* 
 * ioctls()s under CO_MANAGER_IOCTL_MONITOR: 
 */
typedef enum {
	CO_MONITOR_IOCTL_DESTROY,
	CO_MONITOR_IOCTL_LOAD_SECTION, 
	CO_MONITOR_IOCTL_RUN,

	CO_MONITOR_IOCTL_ATTACH_CONSOLE, 
	CO_MONITOR_IOCTL_DOWNLOAD_CONSOLE, 
	CO_MONITOR_IOCTL_CONSOLE_POLL,
	CO_MONITOR_IOCTL_CONSOLE_CANCEL_POLL,
	CO_MONITOR_IOCTL_CONSOLE_MESSAGES, 
	CO_MONITOR_IOCTL_UPLOAD_CONSOLE, 
	CO_MONITOR_IOCTL_DETACH_CONSOLE, 

	CO_MONITOR_IOCTL_NETWORK_PACKET_POLL, 
	CO_MONITOR_IOCTL_NETWORK_PACKET_CANCEL_POLL,
	CO_MONITOR_IOCTL_NETWORK_PACKET_RECEIVE, 
	CO_MONITOR_IOCTL_NETWORK_PACKET_SEND,

	CO_MONITOR_IOCTL_KEYBOARD, 
	CO_MONITOR_IOCTL_TERMINATE, 
	CO_MONITOR_IOCTL_SYSDEP, 

	CO_MONITOR_IOCTL_STATUS,
} co_monitor_ioctl_op_t;

/* interface for CO_MANAGER_IOCTL_MONITOR: */
typedef struct {
	co_rc_t rc;
	co_id_t id;
	co_monitor_ioctl_op_t op;
	char extra_data[];
} co_manager_ioctl_monitor_t;

typedef enum {
	CO_MANAGER_STATE_NOT_INITIALIZED,
	CO_MANAGER_STATE_INITIALIZED,
} co_manager_state_t;

/* interface for CO_MANAGER_IOCTL_STATUS: */
typedef struct {
	co_manager_state_t state;
	int monitors_count;
} co_manager_ioctl_status_t;

/*
 * Monitor ioctl()s
 */

/* interface for CO_MONITOR_IOCTL_LOAD_SECTION: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	char *user_ptr;
	unsigned long address;
	unsigned long size;
	unsigned long index;
	unsigned char buf[0];
} co_monitor_ioctl_load_section_t;

/* interface for CO_MONITOR_IOCTL_CONSOLE_MESSAGE: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	unsigned long num_messages;
	char data[];
} co_monitor_ioctl_console_messages_t;

typedef enum {
	CO_MONITOR_IOCTL_CONSOLE_MESSAGE_NORMAL,
	CO_MONITOR_IOCTL_CONSOLE_MESSAGE_TERMINATED,
} co_monitor_ioctl_console_message_type_t;

typedef struct {
	unsigned long size;
	co_monitor_ioctl_console_message_type_t type;
	union {
		struct {
			unsigned long reason;
		} terminated;
		char extra_data[1];
	};
} co_monitor_ioctl_console_message_t;

/* interface for CO_MONITOR_IOCTL_ATTACH_CONSOLE: */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	unsigned long size;
} co_monitor_ioctl_attach_console_t;

/* interface for CO_MONITOR_IOCTL_KEYBOARD: */
typedef enum {
	CO_OPERATION_KEYBOARD_ACTION,
} co_operation_keyboard_t;

typedef struct co_monitor_ioctl_keyboard {
	co_manager_ioctl_monitor_t pc;
	co_operation_keyboard_t op;
	co_scan_code_t sc;
	int down;
} co_monitor_ioctl_keyboard_t;

/* interface for CO_MONITOR_IOCTL_NETWORK_PACKET_RECEIVE */
typedef struct {
	co_manager_ioctl_monitor_t pc;
	unsigned long size;
	unsigned long more;
	char data[];
} co_monitor_ioctl_network_receive_t;

/* interface for CO_MONITOR_IOCTL_STATUS */
typedef struct co_monitor_ioctl_status {
	co_manager_ioctl_monitor_t pc;
} co_monitor_ioctl_status_t;

#endif
