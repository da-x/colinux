#ifndef __COLINUX_USER_OSDEP_H__
#define __COLINUX_USER_OSDEP_H__

#include <colinux/common/common.h>
#include <windows.h>

co_rc_t co_os_parse_args(LPSTR szCmdLine, char ***args);
void co_os_free_parsed_args(char **args);

#endif
