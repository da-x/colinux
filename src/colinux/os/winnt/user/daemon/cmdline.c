/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>

#include <colinux/common/common.h>
#include <colinux/os/user/misc.h>

#include "cmdline.h"

void co_winnt_daemon_syntax(void)
{
	co_daemon_print_header();
	co_terminal_print("\n");
	co_terminal_print("The following options are specific to Windows NT/XP/2000:\n");
	co_terminal_print("\n");
	co_terminal_print("      --install-service [name]     Install colinux-daemon.exe as an NT service\n");
	co_terminal_print("                                   (default service name: Cooperative Linux)\n");
	co_terminal_print("      --remove-service [name]      Remove colinux service\n");
	co_terminal_print("                                   (default service name: Cooperative Linux)\n");
	co_terminal_print("      --install-driver             Install the colinux-driver (linux.sys)\n");
	co_terminal_print("      --remove-driver              Install the colinux-driver (linux.sys)\n");
	co_terminal_print("      --status-driver              Show status about the installed/running\n");
	co_terminal_print("                                   driver\n");
}

co_rc_t co_winnt_daemon_parse_args(char **args, co_winnt_parameters_t *winnt_parameters) 
{
	char **param_scan = args;
	const char *option;

	/* Default settings */
	co_snprintf(winnt_parameters->service_name, sizeof(winnt_parameters->service_name), "Cooperative Linux");
	winnt_parameters->install_service = PFALSE;
	winnt_parameters->run_service = PFALSE;
	winnt_parameters->remove_service = PFALSE;
	winnt_parameters->install_driver = PFALSE;
	winnt_parameters->remove_driver = PFALSE;
	winnt_parameters->status_driver = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "--install-service";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->install_service = PTRUE;

			if (!*param_scan)
				break;
			
			if (**param_scan == '-')
				continue;

			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);

			param_scan++;
			continue;
		}

		option = "--remove-service";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->remove_service = PTRUE;

			if (!*param_scan)
				break;
			
			if (**param_scan == '-')
				continue;

			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);
			param_scan++;
			continue;
		}

		option = "--run-service";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->run_service = PTRUE;

			if (!*param_scan)
				break;
			
			if (**param_scan == '-')
				continue;

			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);

			param_scan++;
			continue;
		}
		
		option = "--install-driver";

		if (strcmp(*param_scan, option) == 0) {
			winnt_parameters->install_driver = PTRUE;
			param_scan++;
			continue;
		}
		
		option = "--remove-driver";

		if (strcmp(*param_scan, option) == 0) {
			winnt_parameters->remove_driver = PTRUE;
			param_scan++;
			continue;
		}
		
		option = "--status-driver";

		if (strcmp(*param_scan, option) == 0) {
			winnt_parameters->status_driver = PTRUE;
			param_scan++;
			continue;
		}

		param_scan++;
	}

	return CO_RC(OK);
}
