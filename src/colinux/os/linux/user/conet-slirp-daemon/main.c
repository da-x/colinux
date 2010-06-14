
/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <pthread.h>

#include <colinux/user/monitor.h>
#include <colinux/user/slirp/co_main.h>
#include <colinux/os/user/misc.h>

COLINUX_DEFINE_MODULE("colinux-slirp-net-daemon");

static pthread_mutex_t slirp_mutex;

co_rc_t co_slirp_mutex_init (void)
{
	if (pthread_mutex_init(&slirp_mutex, NULL)) {
		co_terminal_print("conet-slirp-daemon: Mutex creating failed\n");
		return CO_RC(ERROR);
	}
	return CO_RC(OK);
}

void co_slirp_mutex_destroy (void)
{
	pthread_mutex_destroy(&slirp_mutex);
}

void co_slirp_mutex_lock (void)
{
	pthread_mutex_lock(&slirp_mutex);
}

void co_slirp_mutex_unlock (void)
{
	pthread_mutex_unlock(&slirp_mutex);
}

int main(int argc, char *argv[])
{
	co_rc_t rc;

	rc = co_slirp_main(argc, argv);
	if (CO_OK(rc))
		return 0;

	return -1;
}
