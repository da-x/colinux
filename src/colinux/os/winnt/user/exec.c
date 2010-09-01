/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>

#include <stdio.h>

#include <colinux/os/user/exec.h>
#include <colinux/os/user/misc.h>

co_rc_t co_launch_process(int *pid, char *command_line, ...)
{
	BOOL ret;
	char buf[0x200];
	va_list ap;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	va_start(ap, command_line);
	vsnprintf(buf, sizeof(buf), command_line, ap);
	va_end(ap);

	co_debug("executing: %s", buf);

	ret = CreateProcess(NULL,
			    buf,              // Command line.
			    NULL,             // Process handle not inheritable.
			    NULL,             // Thread handle not inheritable.
			    FALSE,            // Set handle inheritance to FALSE.
			    0,                // No creation flags.
			    NULL,             // Use parent's environment block.
			    NULL,             // Use parent's starting directory.
			    &si,              // Pointer to STARTUPINFO structure.
			    &pi);             // Pointer to PROCESS_INFORMATION structure.

	if (!ret) {
		co_terminal_print("error 0x%lx in execution\n", GetLastError());
		return CO_RC(ERROR);
	}

	if (pid)
		*pid = pi.dwProcessId;

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return CO_RC(OK);
}


co_rc_t co_kill_process(int pid)
{
	HANDLE hProcess;
	co_rc_t rc = CO_RC(OK);

	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (!hProcess) {
		DWORD err = GetLastError();

		if (err == ERROR_INVALID_PARAMETER)
			return CO_RC(OK); /* Process is not running */

		co_debug_error("error 0x%lx open process %d", err, pid);
		return CO_RC(ERROR);
	}

	if (!TerminateProcess(hProcess, 0)) {
		co_debug_error("error 0x%lx in temination of pid %d", GetLastError(), pid);
		rc = CO_RC(ERROR);
	}

	CloseHandle(hProcess);

	return rc;
}
