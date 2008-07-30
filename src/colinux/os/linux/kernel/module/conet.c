/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <colinux/kernel/monitor.h>

co_rc_t co_conet_bind_adapter(co_monitor_t *monitor, int conet_unit, char *netcfg_id, int promisc, char macaddr[6])
{
	return CO_RC(ERROR);
}

co_rc_t co_conet_unbind_adapter(co_monitor_t *monitor, int conet_unit)
{
	return CO_RC(ERROR);
}

co_rc_t co_conet_inject_packet_to_adapter(co_monitor_t *monitor, int conet_unit, void *packet_data, int length)
{
	return CO_RC(ERROR);
}
