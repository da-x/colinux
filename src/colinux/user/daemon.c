/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/os/user/file.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/alloc.h>

#include "daemon.h"
#include "manager.h"
#include "monitor.h"
#include "config.h"

co_rc_t co_load_config_file(co_daemon_t *daemon)
{
	co_rc_t rc;
	char *buf = NULL;
	unsigned long size = 0 ;

	rc = co_os_file_load(&daemon->config.config_path, &buf, &size);
	if (!CO_OK(rc)) {
		co_debug("Error loading configuration file\n");
		return rc;
	}
	
	if (size == 0) {
		rc = CO_RC_ERROR;
		co_debug("Error, configuration file size is 0\n");
	} else {
		buf[size-1] = '\0';
		rc = co_load_config(buf, &daemon->config);
	}

	co_os_file_free(buf);
	return rc;
}

void co_daemon_syntax()
{
	co_terminal_print("Cooperative Linux Daemon\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("    colinux-daemon [-h] [-c config.xml] [-d]\n");
	co_terminal_print("\n");
	co_terminal_print("      -h             Show this help text\n");
	co_terminal_print("      -c config.xml  Specify configuration file\n");
	co_terminal_print("                     (default: colinux.default.xml)\n");
	co_terminal_print("      -d             Don't launch and attach a coLinux console on\n");
	co_terminal_print("                     startup\n");
}

co_rc_t co_daemon_parse_args(char **args, co_start_parameters_t *start_parameters)
{
	char **param_scan = args;
	const char *option;

	/* Default settings */
	start_parameters->launch_console = PTRUE;
	start_parameters->show_help = PFALSE;
	snprintf(start_parameters->config_path, 
		 sizeof(start_parameters->config_path), "%s", "default.colinux.xml");

	/* Parse command line */
	while (*param_scan) {
		option = "-c";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			if (!(*param_scan)) {
				co_terminal_print(
					"Parameter of command line option %s not specified\n",
					option);
				return CO_RC(ERROR);
			}

			snprintf(start_parameters->config_path, 
				 sizeof(start_parameters->config_path), 
				 "%s", *param_scan);
		}

		option = "-d";

		if (strcmp(*param_scan, option) == 0) {
			start_parameters->launch_console = PFALSE;
		}

		option = "-h";

		if (strcmp(*param_scan, option) == 0) {
			start_parameters->show_help = PTRUE;
		}

		param_scan++;
	}

	return CO_RC(OK);
}

co_rc_t co_daemon_create(co_start_parameters_t *start_parameters, co_daemon_t **co_daemon_out)
{
	co_rc_t rc;
	co_daemon_t *daemon;
	co_manager_handle_t handle;

	co_debug("Loading driver\n");

	rc = co_os_manager_install();
	if (!CO_OK(rc)) {
		co_debug("Error loading driver: %d\n", rc);
		return rc;
	}
	
	handle = co_os_manager_open();
	if (handle == NULL) {
		co_debug("Error opening device\n");
		rc = CO_RC(ERROR);
		goto out;
	}

	rc = co_manager_init(handle);
	co_os_manager_close(handle);

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
		co_debug("Error loading configuration\n");
		goto out_free;
	}

	*co_daemon_out = daemon;
	return rc;

out_free:
	co_os_free(daemon);
out:
	co_debug("Removing driver\n");
	co_os_manager_remove();
	return rc;
}

void co_daemon_destroy(co_daemon_t *daemon)
{
	co_debug("Closing monitor\n");
	co_user_monitor_close(daemon->monitor);
	co_os_free(daemon);

	co_debug("Removing driver\n");
	co_os_manager_remove();
}

co_rc_t co_daemon_load_symbol(co_daemon_t *daemon, 
			      const char *pathname, unsigned long *address_out)
{
	co_rc_t rc = CO_RC_OK;
	Elf32_Sym *sym;

	sym = co_get_symbol_by_name(&daemon->elf_data, pathname);
	if (sym)
		*address_out = sym->st_value;
	else 
		co_debug("symbol %s not found\n", pathname);

	return rc;
}

co_rc_t co_daemon_create_(co_daemon_t *daemon)
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
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "colinux_start", &import->kernel_colinux_start);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "swapper_pg_dir", &import->kernel_swapper_pg_dir);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "pg0", &import->kernel_pg0);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "pg1", &import->kernel_pg1);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "pg2", &import->kernel_pg2);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "idt_table", &import->kernel_idt_table);
	if (!CO_OK(rc)) 
		goto out;

	rc = co_daemon_load_symbol(daemon, "gdt_table", &import->kernel_gdt_table);
	if (!CO_OK(rc)) 
		goto out;

	create_params.config = daemon->config;

	rc = co_user_monitor_create(&daemon->monitor, &create_params);

out:
	return rc;
}

void co_daemon_destroy_(co_daemon_t *daemon)
{
	co_user_monitor_destroy(daemon->monitor);
}

co_rc_t co_daemon_start_monitor(co_daemon_t *daemon)
{
	co_rc_t rc;
	unsigned long size;

	rc = co_os_file_load(&daemon->config.vmlinux_path, &daemon->buf, &size);
	if (!CO_OK(rc)) {
		co_debug("Error loading vmlinux file (%s)\n", 
			    &daemon->config.vmlinux_path);
		goto out;
	}

	rc = co_elf_image_read(&daemon->elf_data, daemon->buf, size);
	if (!CO_OK(rc)) {
		co_debug("Error reading image (%d bytes)\n", size);
		goto out_free_vmlinux; 
	}

	co_debug("Creating monitor\n");

	rc = co_daemon_create_(daemon);
	if (!CO_OK(rc)) {
		co_debug("Error initializing\n");
		goto out_free_vmlinux;
	}

	rc = co_elf_image_load(daemon);
	if (!CO_OK(rc)) {
		co_debug("Error loading image\n");
		goto out_destroy;
	}

	return rc;

out_destroy:
	co_daemon_destroy_(daemon);

out_free_vmlinux:
	co_os_file_free(daemon->buf);

out:
	return rc;
}

co_rc_t co_daemon_run(co_daemon_t *daemon)
{
	co_debug("Running monitor\n");

	return co_user_monitor_run(daemon->monitor);
}

void co_daemon_end_monitor(co_daemon_t *daemon)
{
	co_debug("Destroying monitor\n");

	co_daemon_destroy_(daemon);
	co_os_file_free(daemon->buf);
}

