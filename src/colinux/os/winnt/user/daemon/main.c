/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <windows.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <colinux/user/daemon.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/pipe.h>
#include <colinux/os/winnt/user/osdep.h>

void co_daemon_debug_udp(char *str)
{
	extern u_short htons(u_short);		//in #include <winsock2.h>
	/*
	 * This code sends UDP packet per debug line.
	 *
	 * Proves useful for investigating hard crashes.
	 *
	 * Make sure you have a fast Ethernet.
	 */

	static int sock = -1;    /* We only need one global socket */
	if (sock == -1) {
		struct sockaddr_in server;
		int ret;
		sock = socket(AF_INET, SOCK_DGRAM, 0);

		bzero((char *) &server, sizeof(server));
		server.sin_family = AF_INET;

		/* 
		 * Hardcoded for the meanwhile.
		 * 
		 * If someone actually uses this, please send a patch
		 * to make this more configurable.
		 */

		server.sin_addr.s_addr = inet_addr("192.168.1.1");
		server.sin_port = htons(5555);
		
		ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
	}

	send(sock, str, strlen(str), 0);
}

void co_daemon_debug(const char *str) 
{
#if (0)
	/* For dire situations... */
	co_daemon_debug_udp()
#endif
}

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR szCmdLine,
		   int iCmdShow) 
{
	co_rc_t rc = CO_RC_OK;
	co_daemon_t *daemon = NULL;
	co_start_parameters_t start_parameters;
	co_manager_handle_t handle;
	char **args = NULL;
	bool_t installed = PFALSE;
	int ret;

	co_terminal_print("Cooperative Linux daemon\n");

	rc = co_os_parse_args(szCmdLine, &args);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_daemon_syntax();
		return -1;
	}

	rc = co_daemon_parse_args(args, &start_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		return -1;
	}

	if (start_parameters.show_help) {
		co_daemon_syntax();
		return 0;
	}

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error, unable to determine if driver is installed (rc %d)\n", rc);
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
				co_terminal_print("daemon: manager is already running\n");

				if (status.state == CO_MANAGER_STATE_INITIALIZED) {
					co_terminal_print("daemon: monitors running: %d\n", status.monitors_count);

					if (status.monitors_count != 0) {
						remove = PFALSE;
					}
				}
				else
					co_terminal_print("daemon: manager not initialized (%d)\n", status.state);

			} else {
				co_terminal_print("daemon: status undetermined\n");
			}

			co_os_manager_close(handle);
		}
		else {
			co_terminal_print("daemon: manager not opened\n");
		}
		
		if (remove) {
			/*
			 * Remove older version of the driver.
			 *
			 * NOTE: In the future we would like to run several instances of
			 * coLinux, so intalling and removing the driver needs to be done
			 * some place else, problably by the setup/uninstall scripts. 
			 */
	
			co_terminal_print("daemon: removing driver leftover\n");

			co_os_manager_remove();
		} else {
			co_terminal_print("daemon: driver cannot be removed, aborting\n");
			return -1;
		}
	}

	co_terminal_print("daemon: installing kernel driver\n");
	rc = co_os_manager_install();
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error installing kernel driver: %d\n", rc);
		return rc;
	} else {
		handle = co_os_manager_open();
		if (handle == NULL) {
			co_terminal_print("daemon: error opening kernel driver\n");
			rc = CO_RC(ERROR);
			goto out;
		}
		
		rc = co_manager_init(handle);
		if (!CO_OK(rc)) {
			co_terminal_print("daemon: error initializing kernel driver\n");
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

	rc = co_daemon_run(daemon);

	co_daemon_end_monitor(daemon);

out_destroy:
	co_daemon_destroy(daemon);

out:
	co_terminal_print("daemon: removing kernel driver\n");
	co_os_manager_remove();

	if (!CO_OK(rc)) {
                if (CO_RC_GET_CODE(rc) == CO_RC_OUT_OF_PAGES) {
			co_terminal_print("daemon: not enough physical memory available (try with a lower setting)\n", rc);
		} else 
			co_terminal_print("daemon: exit code: %x\n", rc);
		ret = -1;
	} else {
		ret = 0;
	}

	co_os_free_parsed_args(args);

	return ret;
}
