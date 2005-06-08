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
#include <colinux/common/libc.h>
#include <colinux/user/daemon.h>
#include <colinux/user/debug.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>

#include "../osdep.h"
#include "cmdline.h"
#include "misc.h"
#include "service.h"
#include "driver.h"

COLINUX_DEFINE_MODULE("colinux-daemon");

static co_daemon_t *g_daemon = NULL;
bool_t co_running_as_service = PFALSE;
static bool_t stoped = PFALSE;

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
		g_daemon->next_reboot_will_shutdown = PTRUE;
		co_daemon_send_ctrl_alt_del(g_daemon);
		stoped = PTRUE;
	}
}

/*
 * co_winnt_daemon_ctrl_handler
 *
 * This callback function is called when the user closes the console window,
 * the system is shuting down or the user is loging off (and other control
 * events, like Ctrl+Break).
 * If we handle it, we must return TRUE, otherwise return false to let
 * windows or other control handler process the event.
 * This is needed because the default handler just calls ExitProcess(),
 * making sh*t out of the colinux system.
 */
static BOOL WINAPI co_winnt_daemon_ctrl_handler( DWORD dwCtrlType )
{
	switch ( dwCtrlType )
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		return TRUE;	// Don't let the user kill us that easily ;)
	case CTRL_LOGOFF_EVENT:	
		// Only shutdown if we are not a service
		if ( co_running_as_service )
		    return FALSE;
		// Shutdown to avoid corrupting the fs
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		// Shutdown CoLinux "gracefully"
		co_winnt_daemon_stop( );
		return TRUE;
	}
	// Let windows or others handle it
	// In reality, there are no more events, so this should never
	// be executed (unless in future windows versions)
	return FALSE;
}

co_rc_t co_winnt_daemon_main(co_start_parameters_t *start_parameters) 
{
	co_rc_t rc = CO_RC_OK;
	int ret;

	if (!start_parameters->config_specified  ||  start_parameters->show_help) {
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return CO_RC(OK);
	}

	co_winnt_affinity_workaround();

	// Don't get aborted ;)
	SetConsoleCtrlHandler( co_winnt_daemon_ctrl_handler, TRUE );

	rc = co_daemon_create(start_parameters, &g_daemon);
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
		} else {
			char buf[0x100];
			co_rc_format_error(rc, buf, sizeof(buf));

			co_terminal_print("daemon: exit code %x\n", rc);
			co_terminal_print("daemon: %s\n", buf);
		}

		ret = CO_RC(ERROR);
	} else {
		ret = CO_RC(OK);
	}

	SetConsoleCtrlHandler( co_winnt_daemon_ctrl_handler, FALSE );

	return ret; 
}

static void co_winnt_help(void)
{
	co_terminal_print("\n");
	co_terminal_print("NOTE: Run without arguments to receive help about command line syntax.\n");
}

co_rc_t co_winnt_main(LPSTR szCmdLine)
{
	co_rc_t rc = CO_RC_OK;
	co_winnt_parameters_t winnt_parameters;
	co_start_parameters_t start_parameters;
	co_command_line_params_t cmdline;
	int argc = 0;
	char **args = NULL;

	co_memset(&start_parameters, 0, sizeof(start_parameters));
	co_memset(&winnt_parameters, 0, sizeof(winnt_parameters));

	co_daemon_print_header();

	rc = co_os_parse_args(szCmdLine, &argc, &args);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_winnt_help();
		return rc;
	}

	rc = co_cmdline_params_alloc(args, argc, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_os_free_parsed_args(args);
		return rc;
	}

	rc = co_winnt_daemon_parse_args(cmdline, &winnt_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_winnt_help();
		return CO_RC(ERROR);
	}

	rc = co_daemon_parse_args(cmdline, &start_parameters);
	if (!CO_OK(rc) || start_parameters.show_help){
		if (!CO_OK(rc)) {
			co_terminal_print("daemon: error parsing parameters\n");
		}
		co_winnt_help();
		return CO_RC(ERROR);
	}

	rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PFALSE);
	if (!CO_OK(rc)) {
		co_winnt_help();
		co_terminal_print("\n");
		co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
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
		if (!start_parameters.config_specified) {
			co_terminal_print("daemon: config not specified\n");
			return CO_RC(ERROR);
		}
		return co_winnt_daemon_install_as_service(winnt_parameters.service_name, szCmdLine);
	}

	if (winnt_parameters.remove_service) {
		return co_winnt_daemon_remove_service(winnt_parameters.service_name);
	}

	if (winnt_parameters.remove_driver) {
		return co_winnt_remove_driver();
	}

	if (winnt_parameters.run_service) {
		co_running_as_service = PTRUE;

		co_terminal_print("colinux: running as service '%s'\n", winnt_parameters.service_name);
		if (start_parameters.launch_console)
		{
			co_terminal_print("colinux: not spawning a console, because we're running as service.\n");
			start_parameters.launch_console = PFALSE;
		}

		return co_winnt_daemon_initialize_service(&start_parameters, winnt_parameters.service_name);
	}

	if (!start_parameters.config_specified){
		if (!start_parameters.cmdline_config) {
			co_daemon_syntax();
			co_winnt_daemon_syntax();
		}
		return CO_RC(ERROR);
	}

	return co_winnt_daemon_main(&start_parameters);
}

HINSTANCE co_current_win32_instance;

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR szCmdLine,
		   int iCmdShow) 
{
	co_rc_t rc;

	co_current_win32_instance = hInstance;
	co_debug_start();

	rc = co_winnt_main(szCmdLine);
	if (!CO_OK(rc))  {
    		co_debug_end();
		return -1;
	}

	co_debug_end();
	return 0;
}
