#include <stdio.h>
#include <windows.h>
#include <stdarg.h>

#include <colinux/common/version.h>
#include <colinux/common/libc.h>
#include <colinux/user/debug.h>
#include <colinux/user/cmdline.h>
#include <colinux/os/user/misc.h>
#include "../osdep.h"

COLINUX_DEFINE_MODULE("colinux-control");

static void syntax(void)
{
}

co_rc_t co_winnt_main(LPSTR szCmdLine) 
{
	co_rc_t rc;
	co_command_line_params_t cmdline;
	int argc = 0;
	char **args = NULL;

	rc = co_os_parse_args(szCmdLine, &argc, &args);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		syntax();
		return CO_RC(ERROR);
	}

	rc = co_cmdline_params_alloc(args, argc, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("daemon: error parsing arguments\n");
		co_os_free_parsed_args(args);
		return CO_RC(ERROR);
	}

	return CO_RC(OK);
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
