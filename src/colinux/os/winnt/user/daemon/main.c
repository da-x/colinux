/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * Service support by Jaroslaw Kowalski <jaak@zd.com.pl>, 2004 (c)
 * Driver service separation by Daniel R. Slater <dan_slater@yahoo.com>, 2004 (c)
 * 
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <windows.h>
#include <stdarg.h>

#include <colinux/common/version.h>
#include <colinux/user/daemon.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/pipe.h>

#include "cmdline.h"
#include "misc.h"
#include "service.h"
#include "driver.h"

static co_daemon_t *g_daemon = NULL;
bool_t co_running_as_service = PFALSE;

/*
 * co_winnt_daemon_stop:
 * 
 * This callback function is called when Windows sends a Stop request to the
 * coLinux service.
 *
 **/
void co_winnt_daemon_stop(void)
{
	if (g_daemon != NULL) {
		co_daemon_send_ctrl_alt_del(g_daemon);
	}
}

co_rc_t co_winnt_daemon_main(char *argv[]) 
{
	co_rc_t rc = CO_RC_OK;
	co_start_parameters_t start_parameters;
	int ret;

	rc = co_daemon_parse_args((char **)argv, &start_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return CO_RC(ERROR);
	}

	if (start_parameters.show_help) {
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return 0;
	}
		
	co_winnt_affinity_workaround();
	
	rc = co_daemon_create(&start_parameters, &g_daemon);
	if (!CO_OK(rc))
		goto out;

	rc = co_daemon_start_monitor(g_daemon);
	if (!CO_OK(rc))
		goto out_destroy;

	rc = co_daemon_run(g_daemon);

	co_daemon_end_monitor(g_daemon);

out_destroy:
	co_daemon_destroy(g_daemon);

out:
	if (!CO_OK(rc)) {
		if (CO_RC_GET_CODE(rc) == CO_RC_OUT_OF_PAGES) {
			co_terminal_print("daemon: not enough physical memory available (try with a lower setting)\n", rc);
		} else 
			co_terminal_print("daemon: exit code: %x\n", rc);

		ret = CO_RC(ERROR);
	} else {
		ret = CO_RC(OK);
	}

	return ret; 
}

co_rc_t co_winnt_main(LPSTR szCmdLine)
{
	co_rc_t rc = CO_RC_OK;
	co_winnt_parameters_t winnt_parameters;
	co_start_parameters_t start_parameters;
	char **args;

	co_daemon_print_header();

	rc = co_os_parse_args(szCmdLine, &args);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_daemon_syntax();
		return CO_RC(ERROR);
	}

	rc = co_daemon_parse_args(args, &start_parameters);
	if (!CO_OK(rc) || start_parameters.show_help){
		if (!CO_OK(rc)) {
			co_terminal_print("daemon: error parsing parameters\n");
		}
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return CO_RC(ERROR);
	}

	rc = co_winnt_daemon_parse_args(args, &winnt_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return CO_RC(ERROR);
	}

	if (winnt_parameters.status_driver) {
		co_winnt_status_driver();
		return CO_RC(OK);
	}

	if (winnt_parameters.install_driver) {
		rc = co_winnt_install_driver();
		if (CO_OK(rc)) {
			co_terminal_print("daemon: driver installed\n");
		}
		return rc;
	}

	if (winnt_parameters.install_service) {
		return co_winnt_daemon_install_as_service(winnt_parameters.service_name, &start_parameters);
	}

	if (winnt_parameters.remove_service) {
		return co_winnt_daemon_remove_service(winnt_parameters.service_name);
	}

	if (winnt_parameters.remove_driver) {
		return co_winnt_remove_driver();
	}

	if (winnt_parameters.run_service) {
		co_rc_t rc;

		co_running_as_service = PTRUE;

		rc = co_winnt_initialize_driver();
		if (rc != CO_RC(OK)) {
			return CO_RC(ERROR);
		}
		
		return co_winnt_daemon_initialize_service(args, winnt_parameters.service_name);
	} 

	return co_winnt_daemon_main_with_driver(args);
}

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR szCmdLine,
		   int iCmdShow) 
{
	co_rc_t rc;

	rc = co_winnt_main(szCmdLine);
	if (!CO_OK(rc)) 
		return -1;

	return 0;
}
