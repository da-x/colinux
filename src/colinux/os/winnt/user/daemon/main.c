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

#include <colinux/common/version.h>
#include <colinux/user/daemon.h>
#include <colinux/user/monitor.h>
#include <colinux/user/manager.h>
#include <colinux/os/user/manager.h>
#include <colinux/os/user/misc.h>
#include <colinux/os/user/pipe.h>
#include <colinux/os/winnt/user/osdep.h>

void co_daemon_debug_udp(char *str)
{
#if (0)
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
#endif
}

void co_winnt_daemon_syntax()
{
	co_terminal_print("\n");
	co_terminal_print("The following options are Windows-specific:\n");
	co_terminal_print("\n");
	co_terminal_print("      -i [service]   Install colinux-daemon.exe as a WinNT service\n");
	co_terminal_print("                     (default service name: CoLinux)\n");
	co_terminal_print("      -r [service]   Remove colinux service\n");
	co_terminal_print("                     (default service name: CoLinux)\n");
}

typedef struct co_winnt_parameters {
	bool_t install_service;
	bool_t remove_service;
	bool_t run_service;
	char service_name[128];
} co_winnt_parameters_t;

co_rc_t co_winnt_parse_args(char **args, co_winnt_parameters_t *winnt_parameters) 
{
	char **param_scan = args;
	const char *option;

	/* Default settings */
	co_snprintf(winnt_parameters->service_name, sizeof(winnt_parameters->service_name), "CoLinux");
	winnt_parameters->install_service = PFALSE;
	winnt_parameters->run_service = PFALSE;
	winnt_parameters->remove_service = PFALSE;

	/* Parse command line */
	while (*param_scan) {
		option = "-i";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->install_service = PTRUE;

			if (!(*param_scan)) {
				break;
			}
			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);
		}

		option = "-r";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->remove_service = PTRUE;

			if (!(*param_scan)) {
				break;
			}
			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);
		}

		option = "--run-service";

		if (strcmp(*param_scan, option) == 0) {
			param_scan++;

			winnt_parameters->run_service = PTRUE;

			if (!(*param_scan)) {
				break;
			}

			co_snprintf(winnt_parameters->service_name, 
				sizeof(winnt_parameters->service_name), 
				"%s", *param_scan);
		}

		param_scan++;
	}

	return CO_RC(OK);
}

static void get_last_win32_error(char *error_message, int buf_size) 
{
	DWORD dwLastError = GetLastError();

	if (!FormatMessage(
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_ARGUMENT_ARRAY,
		    NULL,
		    dwLastError,
		    LANG_NEUTRAL,
		    error_message,
		    buf_size,
		    NULL)) 
	{
		co_snprintf(error_message, buf_size, "GetLastError() = %d", dwLastError);
	}
}

int co_daemon_install_service(const char *service_name, co_start_parameters_t *start_parameters) 
{
	SC_HANDLE schService;
	SC_HANDLE schSCManager;
	char exe_name[512];
	char command[1024];
	char error_message[1024];

	co_terminal_print("Installing service '%s'\n", service_name);
	if (!GetModuleFileName(0, exe_name, sizeof(exe_name))) {
		co_terminal_print("Cannot determine exe name. Install failed.\n");
		return 1;
	}

	schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == 0) {
		co_terminal_print("Cannot open service control manager. Install failed.\n");
		return 1;
	}

	co_snprintf(command, sizeof(command), "\"%s\" --run-service \"%s\" -d -c \"%s\"", exe_name, service_name, start_parameters->config_path);
	co_terminal_print("Service command line: %s\n", command);
	schService = CreateService(schSCManager, service_name, service_name, 
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, 
			SERVICE_ERROR_NORMAL, command, NULL, NULL, NULL, NULL, NULL);

	if (schService != 0) {
		co_terminal_print("Service installed.\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return 0;
	} else {
		get_last_win32_error(error_message, sizeof(error_message));
		co_terminal_print("Failed to install service: %s\n", error_message);
		CloseServiceHandle(schSCManager);
		return 1;
	}
}

int co_daemon_remove_service(const char *service_name) 
{
	SC_HANDLE schService;
	SC_HANDLE schSCManager;
	char exe_name[512];
	char command[1024];
	char error_message[1024];

	co_terminal_print("Removing service '%s'\n", service_name);
	if (!GetModuleFileName(0, exe_name, sizeof(exe_name))) {
		co_terminal_print("Cannot determine exe name. Remove failed.\n");
		return 1;
	}

	co_snprintf(command, sizeof(command), "\"%s\" --run-service %s", exe_name, service_name);

	schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == 0) {
		co_terminal_print("Cannot open service control manager. Remove failed.\n");
		return 1;
	}

	schService = OpenService(schSCManager, service_name, SERVICE_ALL_ACCESS);
	if (schService == 0) {
		get_last_win32_error(error_message, sizeof(error_message));
		co_terminal_print("Failed to remove service. OpenService() failed\n");
		CloseServiceHandle(schSCManager);
		return 1;
	}

	if (!DeleteService(schService))	{
		get_last_win32_error(error_message, sizeof(error_message));
		co_terminal_print("Failed to remove service: %s\n", error_message);
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return 1;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	co_terminal_print("Service '%s' removed successfully.\n", service_name);
	return 0;
}


void co_daemon_debug(const char *str) 
{
#if (0)
	/* For dire situations... */
	co_daemon_debug_udp()
#endif
}

static co_daemon_t *g_daemon = NULL;

/*
 * daemon_stop:
 * 
 * This callback function is called when Windows sends a Stop request to the
 * coLinux service.
 **/
void daemon_stop()
{
	if (g_daemon != NULL) {
		co_daemon_send_ctrl_alt_del(g_daemon);
	}
}

int daemon_main(char *argv[]) 
{
	co_rc_t rc = CO_RC_OK;
	co_start_parameters_t start_parameters;
	co_manager_handle_t handle;
	bool_t installed = PFALSE;
	int ret;

	rc = co_daemon_parse_args(argv, &start_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return -1;
	}

	if (start_parameters.show_help) {
		co_daemon_syntax();
		co_winnt_daemon_syntax();
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

	return ret; 
}

static const char *running_service_name;
static char **daemon_args;
static SERVICE_STATUS ssStatus;
static SERVICE_STATUS_HANDLE   sshStatusHandle;

void co_winnt_sc_report_status(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	if (dwCurrentState == SERVICE_START_PENDING)
		ssStatus.dwControlsAccepted = 0;
	else
		ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	ssStatus.dwCurrentState = dwCurrentState;
	ssStatus.dwWin32ExitCode = dwWin32ExitCode;
	ssStatus.dwWaitHint = dwWaitHint;

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		ssStatus.dwCheckPoint = 0;
	else
		ssStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(sshStatusHandle, &ssStatus);
}

void WINAPI co_winnt_service_control_callback(DWORD dwCtrlCode)
{
	switch(dwCtrlCode)
	{
		case SERVICE_CONTROL_STOP:
			co_winnt_sc_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
			daemon_stop();
			return;

		case SERVICE_CONTROL_INTERROGATE:
			break;

		default:
			break;
	}

	co_winnt_sc_report_status(ssStatus.dwCurrentState, NO_ERROR, 0);
}

void WINAPI service_main(int _argc, char **_argv) 
{
	char exe_name[512];
	char *p;
	int i;

	if (!GetModuleFileName(0, exe_name, sizeof(exe_name)))
	{
		co_terminal_print("Cannot determine exe name. Install failed.\n");
		return;
	}

	p = strrchr(exe_name, '\\');
	if (p != 0)
	{	
		*p = 0;
		SetCurrentDirectory(exe_name);
		OutputDebugString(exe_name);
	};

	sshStatusHandle = RegisterServiceCtrlHandler(running_service_name, co_winnt_service_control_callback);

	if (sshStatusHandle) {
		ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ssStatus.dwServiceSpecificExitCode = 0;

		co_winnt_sc_report_status(SERVICE_RUNNING, NO_ERROR, 3000);

		daemon_main(daemon_args);

		co_winnt_sc_report_status(SERVICE_STOPPED, 0, 0);
	}
}

bool_t co_daemon_initialize_service(const char *service_name) 
{
	SERVICE_TABLE_ENTRY dispatch_table[] = {
		{ (char *)service_name, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ 0, 0 },
	};

	running_service_name = service_name;

	if (!StartServiceCtrlDispatcher(dispatch_table)) {
		return PFALSE;
	} else {
		return PTRUE;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, 
		   HINSTANCE hPrevInstance,
		   LPSTR szCmdLine,
		   int iCmdShow) 
{
	co_rc_t rc = CO_RC_OK;
	co_winnt_parameters_t winnt_parameters;
	co_start_parameters_t start_parameters;
	char **args;

	co_terminal_print("Cooperative Linux Daemon %s (winnt)\n", colinux_version);

	rc = co_os_parse_args(szCmdLine, &args);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_daemon_syntax();
		return -1;
	}

	daemon_args = args;

	rc = co_winnt_parse_args(args, &winnt_parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing parameters\n");
		co_daemon_syntax();
		co_winnt_daemon_syntax();
		return -1;
	}

	if (winnt_parameters.install_service) {
		rc = co_daemon_parse_args(args, &start_parameters);
		if (!CO_OK(rc)) {
			co_terminal_print("daemon: error parsing parameters\n");
			co_daemon_syntax();
			co_winnt_daemon_syntax();
			return -1;
		}

		return co_daemon_install_service(winnt_parameters.service_name, &start_parameters);
	}

	if (winnt_parameters.remove_service) {
		return co_daemon_remove_service(winnt_parameters.service_name);
	}

	if (winnt_parameters.run_service) {
		co_daemon_initialize_service(winnt_parameters.service_name);
	} else {
		daemon_main(args);
	}

	return 0;
}
