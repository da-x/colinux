#include <linux/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <string.h>
#include <sys/ioctl.h>

#include "tap.h"

int tap_set_name(int fd, char *dev)
{
	struct ifreq ifr;
	int err;

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
		return err;

	strncpy(dev, ifr.ifr_name, IFNAMSIZ);

	return 0;
}
