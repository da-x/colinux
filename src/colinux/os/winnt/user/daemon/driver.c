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

#include <windows.h>

#include <colinux/common/common.h>
#include <colinux/common/ioctl.h>
#include <colinux/user/daemon.h>
#include <colinux/user/manager.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/winnt/kernel/driver.h>

#include "main.h"
#include "cmdline.h"
#include "misc.h"
#include "driver.h"

co_rc_t co_winnt_install_driver(void) 
{
	co_rc_t rc = CO_RC_OK;
	bool_t installed = PFALSE;
	co_manager_handle_t handle;

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error, unable to determine if driver is installed (rc %x)\n", rc);
		return CO_RC(ERROR);
	}

	if (installed) {
		co_terminal_print("daemon: driver already installed\n");
		return CO_RC(OK);
	}
	
	rc = co_os_manager_install();
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: cannot install\n");
		return CO_RC(ERROR);
	}
	
	handle = co_os_manager_open();
	if (handle == NULL) {
		co_terminal_print("daemon: error opening kernel driver\n");
		return CO_RC(ERROR);
	}
	
	rc = co_manager_init(handle, PTRUE); /* lazy unload */
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error initializing kernel driver\n");
		co_os_manager_close(handle);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t co_winnt_remove_driver(void) 
{
	co_rc_t rc = CO_RC_OK;
	bool_t installed = PFALSE;
	co_manager_handle_t handle;
	co_manager_ioctl_status_t status = {0, };

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error, unable to determine if the driver is installed (rc %x)\n", rc);
		return CO_RC(ERROR);
	}

	if (!installed) {
		co_terminal_print("daemon: driver not installed\n");
		return CO_RC(OK);
	}

	handle = co_os_manager_open();
	if (!handle) {
		co_terminal_print("daemon: couldn't get driver handle, removing anyway\n");
		co_os_manager_remove();
		return CO_RC(ERROR);
	}		
	
	rc = co_manager_status(handle, &status);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: couldn't get driver status (rc %x)\n", rc);
		co_os_manager_close(handle);
		co_terminal_print("daemon: asuming its an old buggy version, trying to remove it\n");
		co_os_manager_remove();
		return CO_RC(ERROR);
	}		

	if (status.monitors_count != 0) {
		co_terminal_print("daemon: monitors are running, cannot remove driver\n");
		co_os_manager_close(handle);
		return CO_RC(ERROR);
	}

	co_os_manager_close(handle);
	co_os_manager_remove();
	return CO_RC(OK);
}	

co_rc_t co_winnt_daemon_main_with_driver(char *argv[]) 
{
	co_rc_t rc = CO_RC_OK, run_rc;
	bool_t installed = PFALSE;
	co_manager_ioctl_status_t status = {0, };
	co_manager_handle_t handle;
	int failures = 0;

	do {
		rc = co_os_manager_is_installed(&installed);
		if (!CO_OK(rc)) {
			co_terminal_print("daemon: error, unable to determine if driver is installed (rc %x)\n", rc);
			return CO_RC(ERROR);
		}

		if (installed) {
			co_terminal_print("daemon: driver is installed\n");

			handle = co_os_manager_open();
			if (!handle) {
				co_terminal_print("daemon: cannot open driver\n");
				return CO_RC(ERROR);
			}		

			rc = co_manager_status(handle, &status);
			if (!CO_OK(rc)) {
				if (CO_RC_GET_CODE(rc) == CO_LINUX_PERIPHERY_ABI_VERSION) {
					co_terminal_print("daemon: detected an old driver version, trying to remove itn");
				} else {
					co_terminal_print("daemon: detected a buggy driver version, trying to remove it\n");
				}
				co_os_manager_close(handle);
				co_os_manager_remove();
				failures++;
				continue;
			}

			if (status.state == CO_MANAGER_STATE_NOT_INITIALIZED) {
				rc = co_manager_init(handle, PTRUE); /* lazy unload */
				if (!CO_OK(rc)) {
					co_terminal_print("daemon: error initializing kernel driver\n");
					co_os_manager_close(handle);
					return CO_RC(ERROR);
				}
			}
		} else {
			rc = co_os_manager_install();
			if (!CO_OK(rc)) {
				co_terminal_print("daemon: cannot install driver\n");
				return CO_RC(ERROR);
			}

			handle = co_os_manager_open();
			if (handle == NULL) {
				co_terminal_print("daemon: error opening kernel driver\n");
				return CO_RC(ERROR);
			}

			rc = co_manager_init(handle, PFALSE); /* no lazy unload */
			if (!CO_OK(rc)) {
				co_terminal_print("daemon: error initializing kernel driver\n");
				co_os_manager_close(handle);
				return CO_RC(ERROR);
			}
		}

		break;
	} while (failures < 3);

	if (failures >= 3) {
		co_terminal_print("daemon: too many retries, giving up\n");
		return CO_RC(ERROR); 
	}
		
	co_terminal_print("daemon: running\n");

	/*
	 * Now that we know that the driver is installed and initialized, 
	 * we can run.
	 */
	run_rc = co_winnt_daemon_main(argv);
	
	co_terminal_print("daemon: finished running, returned rc: %x\n", run_rc);
	
	rc = co_manager_status(handle, &status);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: cannot get driver status\n");
		co_os_manager_close(handle);
		return CO_RC(ERROR);
	}

	co_debug("daemon: current number of monitors: %x\n", status.monitors_count);
	co_debug("daemon: lazy_unload: %d\n", status.lazy_unload);
	
	if ((status.monitors_count == 0)  &&
	    (status.lazy_unload == PFALSE)) 
	{
		co_os_manager_close(handle);
		co_terminal_print("daemon: removing driver\n");
		co_os_manager_remove();
	}

	if (!CO_OK(run_rc)) {
		rc = run_rc;
	}

	return rc;
}

void co_winnt_status_driver(void) 
{
	co_rc_t rc = CO_RC_OK;
	bool_t installed = PFALSE;
	co_manager_handle_t handle;
	co_manager_ioctl_status_t status = {0, };

	co_terminal_print("daemon: checking if the driver is installed\n");

	rc = co_os_manager_is_installed(&installed);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: unable to determine if driver is installed (rc %x)\n", rc);
		return;
	}

	if (!installed) {
		co_terminal_print("daemon: driver not installed\n");
		return;
	}

	handle = co_os_manager_open();
	if (!handle) {
		co_terminal_print("daemon: couldn't get driver handle\n");
		return;
	}		
	
	rc = co_manager_status(handle, &status);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: couldn't get driver status (rc %x)\n", rc);
		co_os_manager_close(handle);
		return;
	}		

	co_terminal_print("daemon: current number of monitors: %x\n", status.monitors_count);
	co_terminal_print("daemon: lazy_unload: %d\n", status.lazy_unload);
	co_os_manager_close(handle);
}
