/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Service support by Jaroslaw Kowalski <jaak@zd.com.pl>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>

static co_terminal_print_hook_func_t terminal_print_hook;

void co_terminal_print(const char *format, ...)
{
	char buf[0x100];
	va_list ap;
	int len;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	printf("%s", buf);

	if (terminal_print_hook != NULL)
		terminal_print_hook(buf);

	len = strlen(buf);
	while (len > 0  &&  buf[len-1] == '\n')
		buf[len - 1] = '\0';
		
	co_debug_lvl(prints, 11, "prints \"%s\"\n", buf);
}

void co_set_terminal_print_hook(co_terminal_print_hook_func_t func)
{
	terminal_print_hook = func;
}

bool_t co_winnt_get_last_error(char *error_message, int buf_size) 
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
		co_snprintf(error_message, buf_size, "GetLastError() = %d\n", dwLastError);
	}

	return dwLastError != 0;
}

void co_terminal_print_last_error(const char *message)
{
	char last_error[0x100];

	if (co_winnt_get_last_error(last_error, sizeof(last_error))) {
		co_terminal_print("%s: last error: %s", message, last_error);
	} else {
		co_terminal_print("%s: success\n", message);
	}
}

bool_t co_winnt_is_winxp_or_better(void)
{
	bool_t bRetVal = PFALSE;
	OSVERSIONINFO info = { sizeof(OSVERSIONINFO) };
	if (GetVersionEx(&info)) {
		if (info.dwMajorVersion > 5)
			bRetVal = PTRUE;
		else if (info.dwMajorVersion == 5 && info.dwMinorVersion > 0)
			bRetVal = PTRUE;
	}

	return bRetVal;
}
