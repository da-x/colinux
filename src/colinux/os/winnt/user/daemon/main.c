/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <windows.h>

#include <colinux/user/daemon.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/winnt/user/osdep.h>

#include "network.h"

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR szCmdLine,
		   int iCmdShow) 
{
	co_rc_t rc = CO_RC_OK;
	co_daemon_t *daemon = NULL;
	conet_daemon_t *net_daemon;
	co_start_parameters_t start_parameters;
	char **args = NULL;
	int ret;

	rc = co_os_parse_args(szCmdLine, &args);
	if (!CO_OK(rc)) {
		co_debug("Error parsing arguments\n");
		co_daemon_syntax();
		return -1;
	}

	rc = co_daemon_parse_args(args, &start_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("Error parsing parameters\n");
		co_daemon_syntax();
		return -1;
	}

	if (start_parameters.show_help) {
		co_daemon_syntax();
		return 0;
	}

	co_os_manager_remove();

	rc = co_daemon_create(&start_parameters, &daemon);
	if (!CO_OK(rc))
		goto out;

	rc = co_daemon_start_monitor(daemon);
	if (!CO_OK(rc))
		goto out_destroy;

	rc = conet_daemon_start(0, &net_daemon);

	rc = co_daemon_run(daemon);

	co_daemon_end_monitor(daemon);

	conet_daemon_stop(net_daemon);

out_destroy:
	co_daemon_destroy(daemon);

out:
	if (!CO_OK(rc)) {
		co_debug("Daemon failed: %d\n", rc);
		ret = -1;
	} else {
		ret = 0;
	}

	co_os_free_parsed_args(args);

	return ret;
}
