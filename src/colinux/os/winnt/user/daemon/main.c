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
	co_manager_handle_t handle;
	char **args = NULL;
	bool_t installed = PFALSE;
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

	co_debug("Cooperative Linux daemon\n");

	/* 
	 * The daemon is currently designed to use a single instance of the
	 * TAP-Win32 driver. This needs to be changed. The configuration XML 
	 * needs to define exactly which TAP devices to use and their names 
	 * according to their appearance in the Control Panel.
	 */

	rc = conet_test_tap();
	if (!CO_OK(rc)) {
		co_terminal_print("Error opening TAP-Win32\n");
		return -1;
	}

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("Error, unable to determine if driver is installed (rc %d)\n", rc);
		return -1;
	}
	
	if (installed) {
		co_manager_handle_t handle;
		bool_t remove = PTRUE;

		handle = co_os_manager_open();
		if (handle) {
			co_manager_ioctl_status_t status = {0, };

			rc = co_manager_status(handle, &status);
			if (CO_OK(rc)) {
				co_debug("Manager is already running\n");

				if (status.state == CO_MANAGER_STATE_INITIALIZED) {
					co_debug("Monitors running: %d\n", status.monitors_count);

					if (status.monitors_count != 0) {
						remove = PFALSE;
					}
				}
				else
					co_debug("Manager not initialized (%d)\n", status.state);

			} else {
				co_debug("Status undetermined\n");
			}

			co_os_manager_close(handle);
		}
		else {
			co_debug("Manager not opened\n");
		}
		
		if (remove) {
			/*
			 * Remove older version of the driver.
			 *
			 * NOTE: In the future we would like to run several instances of
			 * coLinux, so intalling and removing the driver needs to be done
			 * some place else, problably by the setup/uninstall scripts. 
			 */
	
			co_debug("Removing driver leftover\n");

			co_os_manager_remove();
		} else {
			co_debug("Driver cannot be removed, aborting\n");
			return -1;
		}
	}

	co_debug("Installing kernel driver\n");
	rc = co_os_manager_install();
	if (!CO_OK(rc)) {
		co_debug("Error installing kernel driver: %d\n", rc);
		return rc;
	} else {
		handle = co_os_manager_open();
		if (handle == NULL) {
			co_debug("Error opening kernel driver\n");
			rc = CO_RC(ERROR);
			goto out;
		}
		
		rc = co_manager_init(handle);
		if (!CO_OK(rc)) {
			co_debug("Error initializing kernel driver\n");
			co_os_manager_close(handle);
			rc = CO_RC(ERROR);
			goto out;
		}

		co_os_manager_close(handle);
	}

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
	co_debug("Removing kernel driver\n");
	co_os_manager_remove();

	if (!CO_OK(rc)) {
		co_debug("Daemon failed: %d\n", rc);
		ret = -1;
	} else {
		ret = 0;
	}

	co_os_free_parsed_args(args);

	return ret;
}
