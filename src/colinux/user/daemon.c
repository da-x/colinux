/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/common/common.h>
#include <colinux/common/version.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/exec.h>
#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>
#include <colinux/user/macaddress.h>
#include <colinux/user/debug.h>

#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>

#include "daemon.h"
#include "manager.h"
#include "monitor.h"
#include "config.h"
#include "reactor.h"

co_rc_t co_load_config_file(co_daemon_t *daemon)
{
	co_rc_t rc;
	char *buf = NULL;
	unsigned long size = 0 ;

	if (daemon->start_parameters->cmdline_config) {
		daemon->config = daemon->start_parameters->config;
		return CO_RC(OK);
	}

	co_debug("loading configuration from %s\n", daemon->config.config_path);

	rc = co_os_file_load(&daemon->config.config_path, &buf, &size);
	if (!CO_OK(rc)) {
		co_debug("error loading configuration file\n");
		return rc;
	}
	
	if (size == 0) {
		rc = CO_RC_ERROR;
		co_debug("error, configuration file size is 0\n");
	} else {
		buf[size-1] = '\0';
		rc = co_load_config(buf, &daemon->config);
	}

	co_os_file_free(buf);
	return rc;
}

void co_daemon_print_header(void)
{
	static int printed_already = 0;
	if (printed_already)
		return;

	co_terminal_print("Cooperative Linux Daemon, %s\n", colinux_version);
	co_terminal_print("Compiled on %s\n", colinux_compile_time);
	co_terminal_print("\n");
	printed_already = 1;
}

void co_daemon_syntax()
{
	co_daemon_print_header();
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("    colinux-daemon [-h] [-d] [-t name]\n");
	co_terminal_print("                   ([-c config.xml]|[configuration and boot parameters])\n");
	co_terminal_print("\n");
	co_terminal_print("      -h             Show this help text\n");
	co_terminal_print("      -c config.xml  Specify configuration file\n");
	co_terminal_print("                     (default: colinux.default.xml)\n");
	co_terminal_print("      -d             Don't launch and attach a coLinux console on\n");
	co_terminal_print("                     startup\n");
	co_terminal_print("      -k             Suppress kernel messages\n");
	co_terminal_print("      -t name        When spawning a console, this is the type of \n");
	co_terminal_print("                     console (e.g, nt, fltk, etc...)\n");
	co_terminal_print("\n");
	co_terminal_print("      Configuration and boot parameters:\n");
	co_terminal_print("\n");
	co_terminal_print("        When specifying kernel=vmlinux (where vmlinux is the kernel image file\n");
	co_terminal_print("        the -c option is not needed. Instead, you pass all configuration via\n");
	co_terminal_print("        the command line, for example:\n");
	co_terminal_print("\n");
	co_terminal_print("          colinux-daemon kernel=vmlinux cobd0=root_fs root=/dev/cobd0 hda1=:cobd0\n");
	co_terminal_print("\n");
	co_terminal_print("      Use of new aliases automatically allocates cobd(s), for example:\n");
	co_terminal_print("\n");
	co_terminal_print("          colinux-daemon mem=32 kernel=vmlinux hda1=root_fs root=/dev/hda1\n");
	co_terminal_print("\n");
	co_terminal_print("      Unhandled paramters are forwarded to the kernel's boot parameters string.\n");
	co_terminal_print("\n");
}

co_rc_t co_daemon_parse_args(co_command_line_params_t cmdline, co_start_parameters_t *start_parameters)
{
	co_rc_t rc;
	bool_t dont_launch_console = PFALSE;

	start_parameters->show_help = PFALSE;
	start_parameters->config_specified = PFALSE;
	start_parameters->suppress_printk = PFALSE;

	co_snprintf(start_parameters->console, sizeof(start_parameters->console), "fltk");

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-c", 
						      &start_parameters->config_specified,
						      start_parameters->config_path,
						      sizeof(start_parameters->config_path));
	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-t", 
						      NULL,
						      start_parameters->console,
						      sizeof(start_parameters->console));
	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-d", &dont_launch_console);

	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-k", &start_parameters->suppress_printk);

	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-h", &start_parameters->show_help);

	if (!CO_OK(rc)) 
		return rc;

	start_parameters->launch_console = !dont_launch_console;

	rc = co_parse_config_args(cmdline, start_parameters);
	
	return CO_RC(OK);
}

static void init_srand(void)
{
	co_timestamp_t t;

	co_os_get_timestamp(&t);

	srand(t.low ^ t.high);
}

co_rc_t co_daemon_create(co_start_parameters_t *start_parameters, co_daemon_t **co_daemon_out)
{
	co_rc_t rc;
	co_daemon_t *daemon;

	init_srand();

	daemon = (co_daemon_t *)co_os_malloc(sizeof(co_daemon_t));
	if (daemon == NULL) {
		rc = CO_RC(ERROR);
		goto out;
	}

	memset(daemon, 0, sizeof(*daemon));

	daemon->start_parameters = start_parameters;
	memcpy(daemon->config.config_path, start_parameters->config_path, 
	       sizeof(start_parameters->config_path));

	rc = co_load_config_file(daemon);
	if (!CO_OK(rc)) {
		co_debug("error loading configuration\n");
		goto out_free;
	}

	*co_daemon_out = daemon;
	return rc;

out_free:
	co_os_free(daemon);

out:
	return rc;
}

void co_daemon_destroy(co_daemon_t *daemon)
{
	co_debug("daemon cleanup\n");
	co_os_free(daemon);
}

co_rc_t co_daemon_load_symbol_and_data(co_daemon_t *daemon, 
				       const char *symbol_name, 
				       unsigned long *address_out,
				       void *buffer,
				       unsigned long buffer_size)
{
	co_rc_t rc = CO_RC_OK;
	co_elf_symbol_t *sym;
	void *data;

	sym = co_get_symbol_by_name(daemon->elf_data, symbol_name);
	if (sym) 
		*address_out = co_elf_get_symbol_value(sym);
	else {
		co_debug("symbol %s not found\n", symbol_name);
		return CO_RC(ERROR);
		
	}
	
	data = co_elf_get_symbol_data(daemon->elf_data, sym);
	if (data == NULL) {
		co_debug("data of symbol %s not found\n");
		return CO_RC(ERROR);
	}
	
	memcpy(buffer, data, buffer_size);

	return rc;
}

co_rc_t co_daemon_load_symbol(co_daemon_t *daemon, 
			      const char *symbol_name, 
			      unsigned long *address_out)
{
	co_rc_t rc = CO_RC_OK;
	co_elf_symbol_t *sym;

	sym = co_get_symbol_by_name(daemon->elf_data, symbol_name);
	if (sym) 
		*address_out = co_elf_get_symbol_value(sym);
	else {
		co_debug("symbol %s not found\n", symbol_name);
		rc = CO_RC(ERROR);
	}

	return rc;
}

void co_daemon_prepare_net_macs(co_daemon_t *daemon)
{
	int i;

	for (i=0; i < CO_MODULE_MAX_CONET; i++) { 
		co_netdev_desc_t *net_dev;

		net_dev = &daemon->config.net_devs[i];
		if (net_dev->enabled == PFALSE)
			continue;
			
		if (net_dev->manual_mac_address == PFALSE) {
			unsigned long rand_mac = rand();
			int i;

			for (i=0; i < 10; i++)
				rand_mac *= rand() + 1234;

			net_dev->mac_address[0] = 0;
			net_dev->mac_address[1] = 0xFF;
			net_dev->mac_address[2] = rand_mac >> 030;
			net_dev->mac_address[3] = rand_mac >> 020;
			net_dev->mac_address[4] = rand_mac >> 010;
			net_dev->mac_address[5] = rand_mac >> 000;
		}
	}
}

co_rc_t co_load_initrd(co_daemon_t *daemon)
{
	co_rc_t rc = CO_RC(OK);
	char *initrd;
	unsigned long initrd_size; 

	if (!daemon->config.initrd_enabled)
		return rc;

	co_debug("reading initrd from (%s)\n", daemon->config.initrd_path);

	rc = co_os_file_load(&daemon->config.initrd_path, &initrd, &initrd_size);
	if (!CO_OK(rc))
		return rc;

	co_debug("initrd size: %d bytes\n", initrd_size);

	rc = co_user_monitor_load_initrd(daemon->monitor, initrd, initrd_size);

	co_os_free(initrd);

	return rc;
}

static void 
memory_usage_limit_resached(co_manager_ioctl_create_t *create_params)
{
	co_manager_ioctl_info_t info = {0, };
	co_manager_handle_t handle;
	co_rc_t rc;

	co_terminal_print("colinux: memory size configuration for this VM: %d MB\n", 
			  create_params->actual_memsize_used / 0x100000);
	co_terminal_print("colinux: memory usage limit reached\n");
	co_terminal_print("colinux: try to decrease memory size configuration\n");

	handle = co_os_manager_open();
	if (!handle) {
		co_terminal_print("colinux: error opening manager\n");
		return;
	}

	rc = co_manager_info(handle, &info);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux: erroneous manager info\n");
		co_os_manager_close(handle);
		return;
	}
	co_os_manager_close(handle);

	co_terminal_print("colinux: memory usage limit: %d MB\n", 
			  info.hostmem_usage_limit / 0x100000);
	co_terminal_print("colinux: current memory used by running VMs: %d MB\n", 
			  info.hostmem_used / 0x100000);
}

co_rc_t co_daemon_monitor_create(co_daemon_t *daemon)
{
	co_manager_ioctl_create_t create_params = {0, };
	co_symbols_import_t *import;
	co_rc_t rc;

	import = &create_params.import;
	
	rc = co_daemon_load_symbol(daemon, "_kernel_start", &import->kernel_start);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "_end", &import->kernel_end);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "init_thread_union", &import->kernel_init_task_union);
	if (!CO_OK(rc)) {
		goto out;
	}

	rc = co_daemon_load_symbol(daemon, "co_start", &import->kernel_colinux_start);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "swapper_pg_dir", &import->kernel_swapper_pg_dir);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "idt_table", &import->kernel_idt_table);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "cpu_gdt_table", &import->kernel_gdt_table);
	if (!CO_OK(rc))
		goto out;

	rc = co_daemon_load_symbol_and_data(daemon, "co_info", &import->kernel_co_info,
					    &create_params.info, sizeof(create_params.info));
	if (!CO_OK(rc)) {
		goto out;
	}

	rc = co_daemon_load_symbol_and_data(daemon, "co_arch_info", &import->kernel_co_arch_info,
					    &create_params.arch_info, sizeof(create_params.arch_info));
	if (!CO_OK(rc)) {
		goto out;
	}

	if (create_params.info.api_version != CO_LINUX_API_VERSION) {
		co_terminal_print("colinux: error, expected kernel API version %d, got %d\n", CO_LINUX_API_VERSION,
				  create_params.info.api_version);

		rc = CO_RC(VERSION_MISMATCHED);
		goto out;
	}

	if ((create_params.info.compiler_major != __GNUC__) || 
	    (create_params.info.compiler_minor != __GNUC_MINOR__)) {
		co_terminal_print("colinux: error, expected gcc version %d.%d.x, got %d.%d.x\n", __GNUC__,
		 __GNUC_MINOR__,
		 create_params.info.compiler_major,
		 create_params.info.compiler_minor);

		rc = CO_RC(COMPILER_MISMATCHED);
		goto out;
	}

	co_daemon_prepare_net_macs(daemon);

	create_params.config = daemon->config;

	rc = co_user_monitor_create(&daemon->monitor, &create_params);
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_HOSTMEM_USE_LIMIT_REACHED) { 
			memory_usage_limit_resached(&create_params);
		}
		goto out;
	}

	daemon->shared = (co_monitor_user_kernel_shared_t *)create_params.shared_user_address;
	if (!daemon->shared)
		return CO_RC(ERROR); 

	daemon->shared->userspace_msgwait_count = 0;
	daemon->id = create_params.id;

	rc = co_load_initrd(daemon);

out:
	return rc;
}

void co_daemon_monitor_destroy(co_daemon_t *daemon)
{
	co_user_monitor_close(daemon->monitor);
	daemon->monitor = NULL;
}

co_rc_t co_daemon_start_monitor(co_daemon_t *daemon)
{
	co_rc_t rc;
	unsigned long size;

	rc = co_os_file_load(&daemon->config.vmlinux_path, &daemon->buf, &size);
	if (!CO_OK(rc)) {
		co_debug("error loading vmlinux file (%s)\n", 
		      &daemon->config.vmlinux_path);
		goto out;
	}

	rc = co_elf_image_read(&daemon->elf_data, daemon->buf, size);
	if (!CO_OK(rc)) {
		co_debug("error reading image (%d bytes)\n", size);
		goto out_free_vmlinux; 
	}

	co_debug("creating monitor\n");

	rc = co_daemon_monitor_create(daemon);
	if (!CO_OK(rc)) {
		co_debug("error initializing\n");
		goto out_free_vmlinux;
	}

	rc = co_elf_image_load(daemon);
	if (!CO_OK(rc)) {
		co_debug("error loading image\n");
		goto out_destroy;
	}

	return rc;

out_destroy:
	co_daemon_monitor_destroy(daemon);

out_free_vmlinux:
	co_os_file_free(daemon->buf);

out:
	return rc;
}

void co_daemon_send_ctrl_alt_del(co_daemon_t *daemon)
{
}

co_rc_t co_daemon_handle_printk(co_daemon_t *daemon, co_message_t *message)
{
	if (message->type == CO_MESSAGE_TYPE_STRING) {
		char *string_start = message->data;
               
		if (string_start[0] == '<'  &&  
		    string_start[1] >= '0'  &&  string_start[1] <= '9'  &&
		    string_start[2] == '>')
		{
			string_start += 3;
		}

		if (!daemon->start_parameters->suppress_printk) {
			co_terminal_print_color(CO_TERM_COLOR_WHITE, 
						"%s", string_start);
		}

		if (co_strstr(string_start, "VFS: Unable to mount root fs on")) {
			co_terminal_print_color(CO_TERM_COLOR_YELLOW, 
						"colinux: kernel panic suggests that either you forget to supply a\n");
			co_terminal_print_color(CO_TERM_COLOR_YELLOW, 
						"root= kernel boot paramter or the file / device mapped to the root\n");
			co_terminal_print_color(CO_TERM_COLOR_YELLOW, 
						"file system is not found or inaccessible. Please Check your.\n");
			co_terminal_print_color(CO_TERM_COLOR_YELLOW, 
						"coLinux configuration.\n");
		}
	}

	return CO_RC(OK);
}

static co_rc_t message_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	co_message_t *message;
	unsigned long message_size;
	long size_left = size;
	long position = 0;
	co_daemon_t *daemon = (typeof(daemon))(user->private_data);

	while (size_left > 0) {
		message = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left -= message_size;
		if (size_left >= 0) {
			switch(message->to) {
			case CO_MODULE_PRINTK:
				co_daemon_handle_printk(daemon, message);
			default:
				break;
			}
		}
		position += message_size;
	}

	return CO_RC(OK);
}

co_rc_t co_daemon_launch_net_daemons(co_daemon_t *daemon)
{
	int i;
	co_rc_t rc = CO_RC(OK);

	for (i=0; i < CO_MODULE_MAX_CONET; i++) { 
		co_netdev_desc_t *net_dev;
		char interface_name[CO_NETDEV_DESC_STR_SIZE + 0x10] = {0, };

		net_dev = &daemon->config.net_devs[i];
		if (net_dev->enabled == PFALSE)
			continue;
                       
		co_debug("launching daemon for conet%d\n", i);

		if (strlen(net_dev->desc) != 0) {
			co_snprintf(interface_name, sizeof(interface_name), "-n \"%s\"", net_dev->desc);
		}

		switch (net_dev->type) 
		{
		case CO_NETDEV_TYPE_BRIDGED_PCAP: {
			char mac_address[18];
                       
			co_build_mac_address(mac_address, sizeof(mac_address), net_dev->mac_address);

			rc = co_launch_process("colinux-bridged-net-daemon -c %d -i %d %s -mac %s", daemon->id, i, interface_name, mac_address);
			break;
		}

		case CO_NETDEV_TYPE_TAP: {
			rc = co_launch_process("colinux-net-daemon -i %d -u %d %s", daemon->id, i, interface_name);
			break;
		}

		case CO_NETDEV_TYPE_SLIRP: {
			rc = co_launch_process("colinux-slirp-net-daemon -c %d -i %d", daemon->id, i);
			break;
		}

		default:
			rc = CO_RC(ERROR);
			break;
		}

		if (!CO_OK(rc)) {
			co_terminal_print("WARNING: error launching network daemon!\n");
			rc = CO_RC(OK);
		}
	}

	return rc;
}

static co_rc_t co_daemon_restart(co_daemon_t *daemon)
{
	co_rc_t rc = co_user_monitor_reset(daemon->monitor);

	if (!CO_OK(rc)) {
		co_terminal_print("colinux: reset unsuccessful\n");
		return rc;
	}

	rc = co_elf_image_load(daemon);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux: error reloading vmlinux\n");
		return rc;
	}

	rc = co_load_initrd(daemon);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux: error reloading initrd\n");
		return rc;
	}

	return rc;
}

co_rc_t co_daemon_run(co_daemon_t *daemon)
{
	co_rc_t rc;
	co_reactor_t reactor;
	co_user_monitor_t *message_monitor;
	co_module_t modules[] = {CO_MODULE_PRINTK, };
	bool_t restarting = PFALSE;

	rc = co_reactor_create(&reactor);
	if (!CO_OK(rc))
		return rc;

	co_terminal_print("PID: %d\n", daemon->id);

	rc = co_user_monitor_open(reactor, message_receive,
				  daemon->id, modules, sizeof(modules)/sizeof(co_module_t),
				  &message_monitor);

	if (!CO_OK(rc))
		goto out;

	message_monitor->reactor_user->private_data = (void *)daemon;

	if (daemon->start_parameters->launch_console) {
		co_terminal_print("colinux: launching console\n");
		rc = co_launch_process("colinux-console-%s -a %d", daemon->start_parameters->console, daemon->id);
		if (!CO_OK(rc)) {
			co_terminal_print("error launching console\n");
			goto out;
		}
	}

	rc = co_daemon_launch_net_daemons(daemon);
	if (!CO_OK(rc)) {
		co_terminal_print("colinux: launching network daemons\n");
		goto out;
	}

	co_terminal_print("colinux: booting\n");

	do {
		restarting = PFALSE;

		rc = co_user_monitor_start(daemon->monitor);
		if (!CO_OK(rc))
			goto out;
		
		daemon->running = PTRUE;
		while (daemon->running) {
			co_monitor_ioctl_run_t params;
			
			rc = co_user_monitor_run(daemon->monitor, &params);
			if (!CO_OK(rc))
				break;

			co_reactor_select(reactor, 0);
		}

		if (CO_RC_GET_CODE(rc) == CO_RC_INSTANCE_TERMINATED) {
			co_monitor_ioctl_get_state_t params;
			co_termination_reason_t reason;

			co_terminal_print("colinux: Linux VM terminated\n");

			rc = co_user_monitor_get_state(daemon->monitor, &params);
			if (!CO_OK(rc)) {
				co_terminal_print("colinux: unable to get reason for termination (bug?), aborting\n");
				break;
			}

			reason = params.termination_reason;

			switch (reason) {
			case CO_TERMINATE_REBOOT:
				co_terminal_print("colinux: rebooted.\n");
				rc = co_daemon_restart(daemon);
				if (CO_OK(rc))
					restarting = PTRUE;

				break;

			case CO_TERMINATE_POWEROFF:
				co_terminal_print("colinux: powered off, exiting.\n");
				break;

			case CO_TERMINATE_HALT:
				co_terminal_print("colinux: halted, exiting.\n");
				break;

			default:
				co_terminal_print("colinux: terminated with code %d - abnormal exit, aborting\n", reason);
				break;
			}
		}
	} while (restarting);

	co_user_monitor_close(message_monitor);

out:
	co_reactor_destroy(reactor);

	return rc;
}

void co_daemon_end_monitor(co_daemon_t *daemon)
{
	co_debug("shutting down\n");

	co_daemon_monitor_destroy(daemon);
	co_os_file_free(daemon->buf);
}
