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
#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/pipe.h>
#include <colinux/os/user/exec.h>
#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>
#include <colinux/user/macaddress.h>
#include <colinux/user/debug.h>

#include <memory.h>
#include <stdarg.h>

#include "daemon.h"
#include "manager.h"
#include "monitor.h"
#include "config.h"

static void debug(co_daemon_t *daemon, const char *format, ...)
{
	char buf[0x100];
	va_list ap;

	co_snprintf(buf, sizeof(buf), "daemon: %s", format);
	va_start(ap, format);
	co_vdebug(buf, ap);
	va_end(ap);
}

co_rc_t co_load_config_file(co_daemon_t *daemon)
{
	co_rc_t rc;
	char *buf = NULL;
	unsigned long size = 0 ;

	co_terminal_print("daemon: loading configuration from %s\n", daemon->config.config_path);
	rc = co_os_file_load(&daemon->config.config_path, &buf, &size);
	if (!CO_OK(rc)) {
		debug(daemon, "error loading configuration file\n");
		return rc;
	}
	
	if (size == 0) {
		rc = CO_RC_ERROR;
		debug(daemon, "error, configuration file size is 0\n");
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
	co_terminal_print("    colinux-daemon [-h] [-c config.xml] [-d]\n");
	co_terminal_print("\n");
	co_terminal_print("      -h             Show this help text\n");
	co_terminal_print("      -c config.xml  Specify configuration file\n");
	co_terminal_print("                     (default: colinux.default.xml)\n");
	co_terminal_print("      -d             Don't launch and attach a coLinux console on\n");
	co_terminal_print("                     startup\n");
	co_terminal_print("      -t name        When spawning a console, this is the type of \n");
	co_terminal_print("                     console (e.g, nt, fltk, etc...)\n");

}

co_rc_t co_daemon_parse_args(co_command_line_params_t cmdline, co_start_parameters_t *start_parameters)
{
	co_rc_t rc;
	bool_t dont_launch_console = PFALSE;

	start_parameters->show_help = PFALSE;
	start_parameters->config_specified = PFALSE;

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

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-h", &start_parameters->show_help);

	if (!CO_OK(rc)) 
		return rc;

	start_parameters->launch_console = !dont_launch_console;
	
	return CO_RC(OK);
}

co_rc_t co_daemon_create(co_start_parameters_t *start_parameters, co_daemon_t **co_daemon_out)
{
	co_rc_t rc;
	co_daemon_t *daemon;

	daemon = (co_daemon_t *)co_os_malloc(sizeof(co_daemon_t));
	if (daemon == NULL) {
		rc = CO_RC(ERROR);
		goto out;
	}

	memset(daemon, 0, sizeof(*daemon));

	co_list_init(&daemon->connected_modules);
	co_queue_init(&daemon->up_queue);

	daemon->start_parameters = start_parameters;
	memcpy(daemon->config.config_path, start_parameters->config_path, 
	       sizeof(start_parameters->config_path));

        rc = co_console_create(80, 25, 25, &daemon->console);
        if (!CO_OK(rc))
                goto out_free;

	rc = co_load_config_file(daemon);
	if (!CO_OK(rc)) {
		debug(daemon, "error loading configuration\n");
		goto out_free_console;
	}

	*co_daemon_out = daemon;
	return rc;

out_free_console:
	co_console_destroy(daemon->console);
	
out_free:
	co_os_free(daemon);

out:
	return rc;
}

void co_daemon_destroy(co_daemon_t *daemon)
{
	debug(daemon, "daemon cleanup\n");
	if (daemon->console != NULL)
		co_console_destroy(daemon->console);
	co_os_free(daemon);
}

co_rc_t co_daemon_load_symbol(co_daemon_t *daemon, 
			      const char *pathname, unsigned long *address_out)
{
	co_rc_t rc = CO_RC_OK;
	Elf32_Sym *sym;

	sym = co_get_symbol_by_name(&daemon->elf_data, pathname);
	if (sym) 
		*address_out = sym->st_value;
	else {
		debug(daemon, "symbol %s not found\n", pathname);
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

			/*
			 * Pick a MAC address based on device index.
			 */

			net_dev->mac_address[0] = 0;
			net_dev->mac_address[1] = 'C';
			net_dev->mac_address[2] = 'O';
			net_dev->mac_address[3] = 'N';
			net_dev->mac_address[4] = 'E';
			net_dev->mac_address[5] = '0' + i;
		}
	}
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

	rc = co_daemon_load_symbol(daemon, "init_task_union", &import->kernel_init_task_union);
	if (!CO_OK(rc)) {
		rc = co_daemon_load_symbol(daemon, "init_thread_union", &import->kernel_init_task_union);
		if (!CO_OK(rc)) {
			goto out;
		}
	}

	rc = co_daemon_load_symbol(daemon, "colinux_start", &import->kernel_colinux_start);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "swapper_pg_dir", &import->kernel_swapper_pg_dir);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "idt_table", &import->kernel_idt_table);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "gdt_table", &import->kernel_gdt_table);
	if (!CO_OK(rc)) {
		rc = co_daemon_load_symbol(daemon, "cpu_gdt_table", &import->kernel_gdt_table);
		if (!CO_OK(rc))
			goto out;
	}

	co_daemon_prepare_net_macs(daemon);

	create_params.config = daemon->config;

	rc = co_user_monitor_create(&daemon->monitor, &create_params);

out:
	return rc;
}

void co_daemon_monitor_destroy(co_daemon_t *daemon)
{
	co_queue_flush(&daemon->up_queue);
	co_user_monitor_destroy(daemon->monitor);
	co_user_monitor_close(daemon->monitor);
	daemon->monitor = NULL;
}

co_rc_t co_daemon_start_monitor(co_daemon_t *daemon)
{
	co_rc_t rc;
	unsigned long size;

	rc = co_os_file_load(&daemon->config.vmlinux_path, &daemon->buf, &size);
	if (!CO_OK(rc)) {
		debug(daemon, "error loading vmlinux file (%s)\n", 
		      &daemon->config.vmlinux_path);
		goto out;
	}

	rc = co_elf_image_read(&daemon->elf_data, daemon->buf, size);
	if (!CO_OK(rc)) {
		debug(daemon, "error reading image (%d bytes)\n", size);
		goto out_free_vmlinux; 
	}

	debug(daemon, "creating monitor\n");

	rc = co_daemon_monitor_create(daemon);
	if (!CO_OK(rc)) {
		debug(daemon, "error initializing\n");
		goto out_free_vmlinux;
	}

	rc = co_elf_image_load(daemon);
	if (!CO_OK(rc)) {
		debug(daemon, "error loading image\n");
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

co_rc_t co_daemon_handle_console(void *data, co_message_t *message)
{
	co_daemon_t *daemon = (typeof(daemon))(data);
	co_rc_t rc = CO_RC(OK);

	if (daemon->console != NULL) {
		rc = co_console_op(daemon->console, ((co_console_message_t *)message->data));
		co_os_free(message);
	}

	return rc;
}

co_rc_t co_daemon_handle_console_connection(co_connected_module_t *module)
{
	co_console_t *console;
	co_rc_t rc = CO_RC(OK);

	console = module->daemon->console;

	if (console != NULL) {
		struct {
			co_message_t message;
			co_daemon_console_message_t console;
			char data[0];
		} *message;

		module->daemon->console_module = module;
		module->daemon->console = NULL;

		co_console_pickle(console);

		message = (typeof(message))(co_os_malloc(sizeof(*message)+console->size));

		if (message) {
			message->message.to = CO_MODULE_CONSOLE;
			message->message.from = CO_MODULE_DAEMON;
			message->message.size = sizeof(message->console) + console->size;
			message->message.type = CO_MESSAGE_TYPE_STRING;
			message->message.priority = CO_PRIORITY_IMPORTANT;
			message->console.type = CO_DAEMON_CONSOLE_MESSAGE_ATTACH;
			message->console.size = console->size;

			memcpy(message->data, console, console->size);

			rc = co_os_pipe_server_send(module->connection, (char *)message, 
						    sizeof(*message) + console->size);

			co_os_free(message);
		}

		co_console_unpickle(console);
		co_console_destroy(console);
	}

	return rc;
}

co_rc_t co_daemon_handle_console_disconnection(co_connected_module_t *connection)
{
	connection->daemon->console_module = NULL;

	return CO_RC(OK);
}

co_rc_t co_daemon_handle_printk(void *data, co_message_t *message)
{
	if (message->type == CO_MESSAGE_TYPE_STRING) {
		char *string_start = message->data;
		
		if (string_start[0] == '<'  &&  
		    string_start[1] >= '0'  &&  string_start[1] <= '9'  &&
		    string_start[2] == '>')
		{
			string_start += 3;
		}
		co_debug("%s", string_start);
	}

	co_os_free(message);
	return CO_RC(OK);
}

void co_daemon_send_ctrl_alt_del(co_daemon_t *daemon)
{
	struct {
		co_message_t message;
		co_linux_message_t linux_msg;
		co_linux_message_power_t data;
	} message;

	message.message.from = CO_MODULE_DAEMON;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_IMPORTANT;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message.linux_msg) + sizeof(message.data);
	message.linux_msg.device = CO_DEVICE_POWER;
	message.linux_msg.unit = 0;
	message.linux_msg.size = sizeof(message.data);
	message.data.type = CO_LINUX_MESSAGE_POWER_ALT_CTRL_DEL;

	co_message_switch_dup_message(&daemon->message_switch, &message.message);
}

co_rc_t co_daemon_handle_daemon(void *data, co_message_t *message)
{
	co_daemon_t *daemon = (typeof(daemon))(data);

	if (message->from == CO_MODULE_MONITOR) {
		struct {
			co_message_t message;
			co_daemon_message_t payload;
			char data[0];
		} *daemon_message;
		
		daemon_message = (typeof(daemon_message))(message);

		switch (daemon_message->payload.type) {
		case CO_MONITOR_MESSAGE_TYPE_TERMINATED: {
			debug(daemon, "monitor terminated, reason %d\n", daemon_message->payload.terminated.reason);
			daemon->running = PFALSE;
			break;
		}
		case CO_MONITOR_MESSAGE_TYPE_DEBUG_LINE: {
			debug(daemon, "DEBUG: %s", daemon_message->data);
			break;
		}
		case CO_MONITOR_MESSAGE_TYPE_TRACE_POINT: {
			co_daemon_trace_point((co_trace_point_info_t *)&daemon_message->data);
			break;
		}
		}
	} else if (message->from == CO_MODULE_CONSOLE) {
		struct {
			co_message_t message;
			co_daemon_console_message_t console;
			char data[0];
		} *console_message;

		console_message = (typeof(console_message))message;

		if (console_message->console.type == CO_DAEMON_CONSOLE_MESSAGE_DETACH) {
			co_console_t *console;

			console = (co_console_t *)co_os_malloc(console_message->console.size);
			if (console) {
				memcpy(console, console_message->data, console_message->console.size);
				co_console_unpickle(console);				
				daemon->console = console;
			}

		} else if (console_message->console.type == CO_DAEMON_CONSOLE_MESSAGE_TERMINATE) {
			debug(daemon, "termination requested by console\n");
			daemon->running = PFALSE;

		} else if (console_message->console.type == CO_DAEMON_CONSOLE_MESSAGE_CTRL_ALT_DEL) {
			co_daemon_send_ctrl_alt_del(daemon);
		}
	}

	co_os_free(message);
	return CO_RC(OK);
}

co_rc_t co_daemon_handle_idle(void *data, co_message_t *message)
{
	co_daemon_t *daemon = (typeof(daemon))(data);

	daemon->idle = PTRUE;

	co_os_free(message);

	return CO_RC(OK);
}

co_rc_t co_daemon_pipe_cb_connected(co_os_pipe_connection_t *conn,
				    void *data, 
				    void **data_client)
{	
	co_daemon_t *daemon = (typeof(daemon))data;
	co_connected_module_t *module;

	module = co_os_malloc(sizeof(co_connected_module_t));
	if (module == NULL)
		return CO_RC(ERROR);

	module->daemon = daemon;
	module->state = CO_CONNECTED_MODULE_STATE_NEW;
	module->connection = conn;

	co_list_add_head(&module->node, &daemon->connected_modules);
	
	*data_client = module;

	return CO_RC(OK);
}

co_rc_t co_daemon_pipe_cb_packet_send(void *data, co_message_t *message)
{
	co_connected_module_t *module = (typeof(module))(data);
	co_rc_t rc;

	rc = co_os_pipe_server_send(module->connection, (char *)message, sizeof(*message) + message->size);

	co_os_free(message);

	return rc;
}

co_rc_t co_daemon_pipe_cb_packet(co_os_pipe_connection_t *conn, 
				 void **client_data,
				 char *packet_data,
				 unsigned long packet_size)
{
	co_connected_module_t *module = (typeof(module))(*client_data);
	co_rc_t rc = CO_RC(OK);

	switch (module->state) {
		case CO_CONNECTED_MODULE_STATE_NEW: {
			co_module_t id;
			if (packet_size != sizeof(co_module_t)) {
				rc = CO_RC(ERROR);
				break;
			}

			id = *((co_module_t *)packet_data);
				
			if (!((id == CO_MODULE_CONSOLE) ||
			      (CO_MODULE_CONET0 >= id  &&  id < CO_MODULE_CONET_END)))
			{
				rc = CO_RC(ERROR);
				break;
			}
				
			module->id = id;
			module->state = CO_CONNECTED_MODULE_STATE_IDENTIFIED;

			if (id == CO_MODULE_CONSOLE)
				co_snprintf(module->name, sizeof(module->name), "console");
			else
				co_snprintf(module->name, sizeof(module->name), "conet%d", id - CO_MODULE_CONET0);

			debug(module->daemon, "module connected: %s\n", module->name);

			if (module->id == CO_MODULE_CONSOLE)
				co_daemon_handle_console_connection(module);

			rc = co_message_switch_set_rule(&module->daemon->message_switch, id, 
							co_daemon_pipe_cb_packet_send, module);
			if (CO_OK(rc)) {
				struct {
					co_message_t message;
					co_switch_message_t switchm;
				} message;
						
				message.message.from = CO_MODULE_DAEMON;
				message.message.to = CO_MODULE_KERNEL_SWITCH;
				message.message.priority = CO_PRIORITY_DISCARDABLE;
				message.message.type = CO_MESSAGE_TYPE_OTHER;
				message.message.size = sizeof(message.switchm);
				message.switchm.type = CO_SWITCH_MESSAGE_SET_REROUTE_RULE;
				message.switchm.destination = module->id;
				message.switchm.reroute_destination = CO_MODULE_USER_SWITCH;

				co_message_switch_dup_message(&module->daemon->message_switch, &message.message);
			}
			break;
		}
		case CO_CONNECTED_MODULE_STATE_IDENTIFIED: {
			co_message_t *message = (co_message_t *)packet_data;

			/* Validate mesasge size */
			if (message->size + sizeof(*message) != packet_size) {
				debug(module->daemon, "invalid message: %d != %d\n", message->size + sizeof(*message), 
				      packet_size);
				break;
			}

			/* Prevent module impersonation */
			if (message->from != module->id) {
				debug(module->daemon, "invalid message id: %d != %d\n", 
				      message->from, module->id);
				break;
			}

			co_message_switch_dup_message(&module->daemon->message_switch, message);
			break;
		}
	default:
		rc = CO_RC(ERROR);
		break;
	}

	return rc;
}

void co_daemon_pipe_cb_disconnected(co_os_pipe_connection_t *conn,
				    void **client_data)
{
	co_connected_module_t *module = (typeof(module))(*client_data);

	if (module->state == CO_CONNECTED_MODULE_STATE_IDENTIFIED) {
		struct {
			co_message_t message;
			co_switch_message_t switchm;
		} message;

		message.message.from = CO_MODULE_DAEMON;
		message.message.to = CO_MODULE_KERNEL_SWITCH;
		message.message.priority = CO_PRIORITY_DISCARDABLE;
		message.message.type = CO_MESSAGE_TYPE_OTHER;
		message.message.size = sizeof(message.switchm);
		message.switchm.type = CO_SWITCH_MESSAGE_FREE_RULE;
		message.switchm.destination = module->id;
		
		co_message_switch_dup_message(&module->daemon->message_switch, &message.message);
		co_message_switch_free_rule(&module->daemon->message_switch, module->id);

		debug(module->daemon, "module disconnected: %s\n", module->name);

		if (module->id == CO_MODULE_CONSOLE)
			co_daemon_handle_console_disconnection(module);
	}
	
	co_list_del(&module->node);
	co_os_free(module);
}

/*
 * This functions sends timer interrupt messages to coLinux every 1/HZ
 * seconds.
 */ 

void co_daemon_idle(void *data)
{
	co_daemon_t *daemon = (typeof(daemon))data;
	struct {
		co_message_t message;
		co_linux_message_t linux_msg;
		co_linux_message_idle_t data;
	} message;
	double this_htime;
	double reminder;

	message.message.from = CO_MODULE_DAEMON;
	message.message.to = CO_MODULE_LINUX;
	message.message.priority = CO_PRIORITY_DISCARDABLE;
	message.message.type = CO_MESSAGE_TYPE_OTHER;
	message.message.size = sizeof(message.linux_msg) + sizeof(message.data);
	message.linux_msg.device = CO_DEVICE_TIMER;
	message.linux_msg.unit = 0;
	message.linux_msg.size = sizeof(message.data);
	message.data.tick_count = 0;

	this_htime = co_os_timer_highres();
	reminder = this_htime - daemon->last_htime + daemon->reminder_htime;

	if (reminder < 0) {
		co_debug("co_daemon_idle: Time going backwards? (%.7f)\n", reminder);
		reminder = 0;
	}

	while (reminder >= 0.010) {
		reminder -= 0.010;
		message.data.tick_count++;
	}

	if (message.data.tick_count > 0) 
		co_message_switch_dup_message(&daemon->message_switch, &message.message);

	daemon->reminder_htime = reminder;
	daemon->last_htime = this_htime;
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
			
		debug(daemon, "launching daemon for conet%d\n", i);

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
			rc = co_launch_process("colinux-net-daemon -c %d %s -i %d", daemon->id, interface_name, i);
			break;
		}

		default:
			rc = CO_RC(ERROR);
			break;
		}

		if (!CO_OK(rc)) {
			co_terminal_print("colinux: WARNING: error launching network daemon!\n");
			rc = CO_RC(OK);
		}
	}

	return rc;
}

co_rc_t co_daemon_run(co_daemon_t *daemon)
{
	co_rc_t rc;
	co_os_pipe_server_t *ps;
	co_id_t id = 0;

	rc = co_user_monitor_start(daemon->monitor);
	if (!CO_OK(rc))
		return rc;

	rc = co_os_pipe_server_create(
		co_daemon_pipe_cb_connected,
		co_daemon_pipe_cb_packet,
		co_daemon_pipe_cb_disconnected,
		daemon, &ps, &id);

	if (!CO_OK(rc))
		return rc;

	co_debug("colinux: allocated id %d\n", id);
	
	daemon->id = id;
	daemon->last_htime = co_os_timer_highres();
	daemon->reminder_htime = 0;

	co_message_switch_init(&daemon->message_switch, CO_MODULE_USER_SWITCH);

	rc = co_message_switch_set_rule(&daemon->message_switch, CO_MODULE_PRINTK, 
					co_daemon_handle_printk, daemon);

	if (!CO_OK(rc))
		goto out;

	rc = co_message_switch_set_rule(&daemon->message_switch, CO_MODULE_DAEMON, 
					co_daemon_handle_daemon, daemon);


	if (!CO_OK(rc))
		goto out;

	rc = co_message_switch_set_rule(&daemon->message_switch, CO_MODULE_CONSOLE,
					co_daemon_handle_console, daemon);

	if (!CO_OK(rc))
		goto out;


	rc = co_message_switch_set_rule(&daemon->message_switch, CO_MODULE_IDLE, 
					co_daemon_handle_idle, daemon);


	if (!CO_OK(rc))
		goto out;

	rc = co_message_switch_set_rule_queue(&daemon->message_switch, CO_MODULE_LINUX, 
					      &daemon->up_queue);
	
	if (!CO_OK(rc))
		goto out;

	rc = co_message_switch_set_rule_queue(&daemon->message_switch, CO_MODULE_KERNEL_SWITCH, 
					      &daemon->up_queue);
	
	if (!CO_OK(rc))
		goto out;

	co_terminal_print("colinux: launching net daemons\n");
	rc = co_daemon_launch_net_daemons(daemon);
	if (!CO_OK(rc))
		goto out;

	if (daemon->start_parameters->launch_console) {
		debug(daemon, "launching console\n");
		rc = co_launch_process("colinux-console-%s -a %d", daemon->start_parameters->console, id);
		if (!CO_OK(rc)) {
			co_terminal_print("colinux: error launching console\n");
			goto out;
		}
	}

	daemon->running = PTRUE;

	while (daemon->running) {
		co_monitor_ioctl_run_t *params;
		unsigned long write_size = 0;
		char buf[0x10000];
		char *param_data;
		int i;
		
		params = (typeof(params))buf;
		params->num_messages = 0;

		co_message_write_queue(&daemon->up_queue, params->data, 
				       ((((char *)buf) + sizeof(buf)) - params->data), 
				       &params->num_messages, &write_size);

		rc = co_user_monitor_run(daemon->monitor, params, sizeof(*params) + write_size, sizeof(buf));

		param_data = params->data;

		for (i=0; i < params->num_messages; i++) {
			co_message_t *message = (typeof(message))param_data;
			
			rc = co_message_switch_dup_message(&daemon->message_switch, message);
			
			param_data += message->size + sizeof(*message);
		}

		rc = co_os_pipe_server_service(ps, daemon->idle ? PTRUE : PFALSE);

		co_daemon_idle(daemon);
		daemon->idle = PFALSE;

		if (daemon->send_ctrl_alt_del) {
			co_daemon_send_ctrl_alt_del(daemon);
			daemon->send_ctrl_alt_del = PFALSE;
		}
	}

out:
	co_message_switch_free(&daemon->message_switch);
	co_os_pipe_server_destroy(ps);

	return rc;
}

void co_daemon_end_monitor(co_daemon_t *daemon)
{
	co_terminal_print("colinux: shutting down\n");

	co_daemon_monitor_destroy(daemon);
	co_os_file_free(daemon->buf);
}

