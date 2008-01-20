
/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

#include <colinux/kernel/monitor.h>
#include <linux/cooperative.h>
#include <linux/copci.h>

#define COPCI_MAX_SLOTS	32
#define COPCI_MAX_FUNCS	8

enum COPCI_DEVICE_TYPE {
	COPCI_DT_NONE=0,
	COPCI_DT_VIDEO,
	COPCI_DT_AUDIO,
	COPCI_DT_IDE,
	COPCI_DT_NET,
};

typedef struct {
	unsigned type: 3;			/* Max 6 device types (NONE=1) */
	unsigned unit: 5;			/* Max unit of 31 */
} pci_config_t;

pci_config_t temp[32][8];

void pci_add_dev(pci_config_t **pc, int type, int unit) {
	register int x;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		if (pc[x][0].type == 0) {
			pc[x][0].type = type;
			pc[x][0].unit = unit;
		}
	}
}

int pci_find_dev(pci_config_t **pc, int type, int unit) {
	register int x,y;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		for(y=0; y < COPCI_MAX_FUNCS; y++) {
			if (pc[x][y].type == type && pc[x][y].unit == unit)
				return 1;
		}
	}
	return 0;
}

void pci_chknadd(pci_config_t **pc, void **devs, int max, int type) {
	register int x;

	for(x=0; x < max; x++) {
		if (devs[x]) {
			if (pci_find_dev(pc, type, x) == 0)
				pci_add_dev(pc, type, x);
		}
	}
}

void pci_autoconf(co_monitor_t *cmon) {
//	pci_config_t *pc = &cmon->pci_config;
	pci_config_t **pc = (pci_config_t **) temp;

	pci_chknadd(pc, (void *)&cmon->video_devs, CO_MODULE_MAX_COVIDEO, CO_DEVICE_DISPLAY);
//	pci_chknadd(pc, (void *)&cmon->ide_devs, CO_MODULE_MAX_COIDE, CO_DEVICE_IDE);
	pci_chknadd(pc, (void *)&cmon->scsi_devs, CO_MODULE_MAX_COSCSI, CO_DEVICE_SCSI);
	pci_chknadd(pc, (void *)&cmon->audio_devs, CO_MODULE_MAX_COAUDIO, CO_DEVICE_AUDIO);
	pci_chknadd(pc, (void *)&cmon->config.net_devs, CO_MODULE_MAX_CONET, CO_DEVICE_NETWORK);
}

#if 0
void copci_get_config(co_monitor_t *cmon) {
	copci_config_t *config = &co_passage_page->params[1];
	copci_devinfo_t *devp;
	co_netdev_desc_t *netp;
	register int x;

	config->count = 0;

	/* Video adapters */
	devp = &config->devs[config->count++];
	devp->type = CO_DEVICE_DISPLAY;
	devp->unit = 0;
	devp->irq = 0;

	/* SCSI interfaces */
	for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
		if (cmon->scsi_devs) {
			/* We only do 1 */
			devp = &config->devs[config->count++];
			devp->type = CO_DEVICE_SCSI;
			devp->unit = 0;
			devp->irq = 11;
			break;
		}
	}

	/* Sound cards */
	devp = &config->devs[config->count++];
	devp->type = CO_DEVICE_AUDIO;
	devp->unit = 0;
	devp->irq = 5;

	/* Network interfaces */
	for(x=0; x < CO_MODULE_MAX_CONET; x++) {
		netp = &cmon->config.net_devs[x];
		if (!netp->enabled) continue;
		devp = &config->devs[config->count++];
		devp->type = CO_DEVICE_NETWORK;
		devp->unit = x;
		devp->irq = (x ? 0 : 10);
		co_memcpy(&devp->data, &netp->mac_address, 6);
	}
}
#endif
