/*
 * coLinux
 *
 * Dan Aloni <da-x@colinux.org, 2003 (c)
 */

#include <linux/kernel.h>

#include "block.h"

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

	for (i=0; i < CO_MAX_BLOCK_DEVICES; i++) {
		co_block_dev_t *dev = cmon->block_devs[i];
		if (!dev)
			continue;

		co_monitor_block_unregister_device(cmon, i);
		dev->free(cmon, dev);
	}
}

co_block_dev_t *co_monitor_block_dev_from_index(co_monitor_t *cmon, unsigned long index)
{
	if (index >= CO_MAX_BLOCK_DEVICES)
		return NULL;

	return cmon->block_devs[index];
}

co_rc_t co_monitor_block_request(co_monitor_t *cmon, unsigned long index, 
				 co_block_request_t *request)
{
	co_block_dev_t *dev;
	co_rc_t rc;

	rc = CO_RC_ERROR;

	dev = co_monitor_block_dev_from_index(cmon, index);
	if (!dev)
		return rc;

	rc = CO_RC_OK;

	switch (request->type) { 
	case CO_BLOCK_OPEN: {
		if (dev->use_count >= 1) { 
			dev->use_count++;
			return rc;
		}
		break;
	}
	case CO_BLOCK_CLOSE: {
		if (dev->use_count == 0) {
			rc = CO_RC_ERROR;
			return rc;
		} else if (dev->use_count > 1) {
			dev->use_count--;
			return rc;
		}
		break;
	}
	default:
		break;
	}

	rc = (dev->service)(cmon, dev, request);

	switch (request->type) { 
	case CO_BLOCK_OPEN: {
		if (CO_OK(rc))
			dev->use_count++;
		break;
	}
	case CO_BLOCK_CLOSE: {
		if (CO_OK(rc))
			dev->use_count--;
		break;
	default:
		break;
	}
	}

	return rc;
}
