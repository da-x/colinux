/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 * Alejandro R. Sedeno <asedeno@mit.edu>, 2004 (c)
 * Fabrice Bellard, 2003-2004 Copyright (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <windows.h>
#include <stdint.h>

#include <colinux/user/monitor.h>
#include <colinux/user/slirp/co_main.h>
#include <colinux/os/user/misc.h>

COLINUX_DEFINE_MODULE("colinux-slirp-net-daemon");

static HANDLE slirp_mutex;

co_rc_t co_slirp_mutex_init (void)
{
	slirp_mutex = CreateMutex(NULL, FALSE, NULL);
	if (slirp_mutex == NULL) {
		co_terminal_print("conet-slirp-daemon: Mutex creating failed\n");
		return CO_RC(ERROR);
	}
	return CO_RC(OK);
}

void co_slirp_mutex_destroy (void)
{
	CloseHandle(slirp_mutex);
}

void co_slirp_mutex_lock (void)
{
	WaitForSingleObject(slirp_mutex, INFINITE);
}

void co_slirp_mutex_unlock (void)
{
	ReleaseMutex(slirp_mutex);
}

int main(int argc, char *argv[])
{
	co_rc_t rc;

	rc = co_slirp_main(argc, argv);
	if (CO_OK(rc))
		return 0;

	return -1;
}
