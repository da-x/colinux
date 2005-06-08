#ifndef __COLINUX_USER_OSDEP_H__
#define __COLINUX_USER_OSDEP_H__

#include <colinux/common/common.h>
#include <windows.h>

extern co_rc_t co_os_parse_args(LPSTR szCmdLine, int *count, char ***args);
extern void co_os_free_parsed_args(char **args);

#endif
