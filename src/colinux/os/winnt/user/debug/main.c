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
#include <unistd.h>
#include <stdlib.h>

#include <colinux/user/manager.h>
#include <colinux/user/daemon.h>

COLINUX_DEFINE_MODULE("colinux-debug-daemon");

#define BUFFER_SIZE   (0x100000)

int main(int argc, char *argv[]) 
{
	int ret;

	ret = co_debug_main(argc, argv);

	return ret;
}
