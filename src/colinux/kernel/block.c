/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003-2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/common/libc.h>

#include "block.h"
#include "monitor.h"

void co_monitor_block_register_device(co_monitor_t *cmon, unsigned long index, co_block_dev_t *dev)
{
	cmon->block_devs[index] = dev;
}

void co_monitor_block_unregister_device(co_monitor_t *cmon, unsigned long index)
{
	cmon->block_devs[index] = NULL;
}

void co_monitor_unregister_and_free_block_devices(co_monitor_t *cmon)
{
	long i;

	for (i=0; i < CO_MODULE_MAX_COBD; i++) {
		co_block_dev_t *dev = cmon->block_devs[i];
		if (!dev)
			continue;

		co_monitor_block_unregister_device(cmon, i);
		dev->free(cmon, dev);
	}
}

co_block_dev_t *co_monitor_block_dev_from_index(co_monitor_t *cmon, unsigned long index)
{
	if (index >= CO_MODULE_MAX_COBD)
		return NULL;

	return cmon->block_devs[index];
}

co_rc_t co_monitor_block_request(co_monitor_t *cmon, unsigned long index, 
				 co_block_request_t *request)
{
	co_block_dev_t *dev;
	co_rc_t rc;

	rc = CO_RC(ERROR);

	dev = co_monitor_block_dev_from_index(cmon, index);
	if (!dev)
		return rc;

	rc = CO_RC_OK;

	switch (request->type) { 
	case CO_BLOCK_OPEN: {
		co_debug("cobd%d: open (count=%d)\n", index, dev->use_count);
		if (dev->use_count >= 1) { 
			dev->use_count++;
			return rc;
		}
		break;
	}
	case CO_BLOCK_CLOSE: {
		if (dev->use_count == 0) {
			co_debug("cobd%d: close with no open\n", index);
			rc = CO_RC_ERROR;
			return rc;
		} else if (dev->use_count > 1) {
			dev->use_count--;
			co_debug("cobd%d: close (count=%d)\n", index, dev->use_count);
			return rc;
		}
		co_debug("cobd%d: close (count=%d)\n", index, dev->use_count);
		break;
	}
	case CO_BLOCK_GET_ALIAS: {
		co_debug("cobd%d: %x, %d\n", index, dev, dev->conf);
		if (!dev->conf || !dev->conf->enabled || !dev->conf->alias_used)
			return CO_RC(ERROR);
		dev->conf->alias[sizeof(dev->conf->alias)-1] = '\0';
		co_snprintf(request->alias, sizeof(request->alias), "%s", dev->conf->alias);
		return rc;
	}
	default:
		break;
	}

	rc = (dev->service)(cmon, dev, request);

	switch (request->type) { 
	case CO_BLOCK_OPEN: {
		if (CO_OK(rc)) {
			co_debug("cobd%d: open success (count=%d)\n", index, dev->use_count);
			dev->use_count++;
		} else {
			co_debug("cobd%d: open failed (count=%d, rc=%x)\n", index, dev->use_count, rc);
		}

		break;
	}
	case CO_BLOCK_CLOSE: {
		if (CO_OK(rc)) {
			dev->use_count--;
			co_debug("cobd%d: close success (count=%d)\n", index, dev->use_count);
		} else {
			co_debug("cobd%d: close failed (count=%d, rc=%x)\n", index, dev->use_count, rc);
		}
		break;
	default:
		break;
	}
	}

	return rc;
}
