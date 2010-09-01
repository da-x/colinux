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
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>

#include <colinux/common/libc.h>
#include <colinux/user/daemon.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/user/debug.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>

COLINUX_DEFINE_MODULE("colinux-daemon");

static co_daemon_t *colinux_daemon = NULL;

static void sighup_handler(int sig)
{
	co_terminal_print("daemon: received SIGHUP\n");

	colinux_daemon->send_ctrl_alt_del = PTRUE;
}

static int daemon_main(int argc, char *argv[])
{
	co_rc_t rc = CO_RC_OK;
	co_start_parameters_t start_parameters;
	co_command_line_params_t cmdline;
	co_manager_handle_t handle;
	bool_t installed = PFALSE;
	int ret;

	co_daemon_print_header();

	co_memset(&start_parameters, 0, sizeof(start_parameters));

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		return CO_RC(ERROR);
	}

	rc = co_daemon_parse_args(cmdline, &start_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		return -1;
	}

	if (!start_parameters.config_specified || start_parameters.show_help) {
		co_daemon_syntax();
		return 0;
	}

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error, unable to determine if driver is installed (rc %x)\n", (int)rc);
		return -1;
	}

	if (!installed) {
		co_terminal_print("daemon: error, kernel module is not loaded\n");
		return -1;
	}

	handle = co_os_manager_open();
	if (handle) {
		co_manager_ioctl_status_t status = {0, };

		rc = co_manager_status(handle, &status);
		if (CO_OK(rc)) {
			co_terminal_print("daemon: manager is loaded\n");
		} else {
			if (CO_RC_GET_CODE(rc) == CO_RC_VERSION_MISMATCHED) {
				co_terminal_print("driver version is %d while expected version %d\n",
					 status.periphery_api_version, CO_LINUX_PERIPHERY_API_VERSION);
			}
			co_terminal_print("daemon: can't get manager status\n");
			return -1;
		}
	} else {
		co_terminal_print("daemon: cannot open driver\n");
		return -1;
	}
	co_os_manager_close(handle);

	rc = co_daemon_create(&start_parameters, &colinux_daemon);
	if (!CO_OK(rc))
		goto out;

	rc = co_daemon_start_monitor(colinux_daemon);
	if (!CO_OK(rc))
		goto out_destroy;

	signal(SIGHUP, sighup_handler);
	rc = co_daemon_run(colinux_daemon);
	signal(SIGHUP, SIG_DFL);

	co_daemon_end_monitor(colinux_daemon);

out_destroy:
	co_daemon_destroy(colinux_daemon);
	colinux_daemon = NULL;

out:
	if (!CO_OK(rc)) {
                if (CO_RC_GET_CODE(rc) == CO_RC_OUT_OF_PAGES) {
			co_terminal_print("daemon: not enough physical memory available (try with a lower setting)\n");
		} else {
			char buf[0x100];
			co_rc_format_error(rc, buf, sizeof(buf));

			co_terminal_print("daemon: exit code %08x\n", (int)rc);
			co_terminal_print("daemon: %s\n", buf);
		}
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	int ret;

	co_debug_start();

	ret = daemon_main(argc, argv);

	co_debug_end();

	return ret;
}
