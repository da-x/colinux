
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
#include <colinux/common/libc.h>
#include <colinux/os/alloc.h>
#include <linux/cooperative.h>
#include <linux/copci.h>

static int pci_space_find(co_monitor_t *cmon, int type, int unit) {
	register int x,y;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		for(y=0; y < COPCI_MAX_FUNCS; y++) {
			if (cmon->pci_space[x][y].type == type && cmon->pci_space[x][y].unit == unit)
				return 1;
		}
	}
	return 0;
}

static void pci_space_add(co_monitor_t *cmon, int func, int type, int unit) {
	register int x;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		if (cmon->pci_space[x][0].type == 0 || cmon->pci_space[x][0].type == type) {
			if (pci_space_find(cmon, type, unit) == 0) { 
				if (func && cmon->pci_space[x][0].type == 0) func = 0;
				co_debug("adding type %d to slot %d func %d", type, x, func);
				cmon->pci_space[x][func].type = type;
				cmon->pci_space[x][func].unit = unit;
			}
			break;
		}
	}
}

static void dump_space(co_monitor_t *cmon) {
	register int x,y;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		for(y=0; y < COPCI_MAX_FUNCS; y++) {
			if (cmon->pci_space[x][y].type)
				co_debug("pci_space[%d][%d]: type: %d, unit: %d", x, y,
					cmon->pci_space[x][y].type, cmon->pci_space[x][y].unit);
		}
	}
}

co_rc_t co_pci_setconfig(co_monitor_t *cmon) {
	struct co_pci_desc *pci;
	co_netdev_desc_t *netp;
	register int x,y;

	co_memset(cmon->pci_space, 0, sizeof(cmon->pci_space));

	/* 1st, add the user settings */
	pci = cmon->config.pci;
	while(pci) {
		co_debug("user: dev: %d, func: %d, type: %d, unit: %d", pci->dev, pci->func, pci->type, pci->unit);
		if (pci->func && cmon->pci_space[pci->dev][0].type == 0) pci->func = 0;
		cmon->pci_space[pci->dev][pci->func].type = pci->type;
		cmon->pci_space[pci->dev][pci->func].unit = pci->unit;
		pci = pci->next;
	}
	co_debug("*********** user settings:");
	dump_space(cmon);

	/* Video adapters */
	for(x=0; x < CO_MODULE_MAX_COVIDEO; x++) {
		if (cmon->video_devs[x]) {
			co_debug("adding video%d", x);
			pci_space_add(cmon, 0, CO_DEVICE_VIDEO, x);
		}
	}

	/* Audio adapters */
	for(x=0; x < CO_MODULE_MAX_COAUDIO; x++) {
		if (cmon->audio_devs[x]) {
			co_debug("adding audio%d", x);
			pci_space_add(cmon, 0, CO_DEVICE_VIDEO, x);
		}
	}

	/* SCSI interfaces */
	for(x=0; x < CO_MODULE_MAX_COSCSI; x++) {
		if (cmon->scsi_devs[x]) {
			co_debug("adding scsi%d", 0);
			pci_space_add(cmon, 0, CO_DEVICE_SCSI, 0);
			break;
		}
	}

#ifdef CO_DEVICE_IDE
	/* IDE interfaces */
	for(x=0; x < CO_MODULE_MAX_IDE; x++) {
		if (cmon->ide_devs[x]) {
			co_debug("adding ide%d", x);
			pci_space_add(cmon, 0, CO_DEVICE_IDE, 0);
			break;
		}
	}
#endif

	/* Network interfaces */
	for(x=y=0; x < CO_MODULE_MAX_CONET; x++) {
		netp = &cmon->config.net_devs[x];
		if (!netp->enabled) continue;
		co_debug("adding net%d", x);
		pci_space_add(cmon, y++, CO_DEVICE_NETWORK, x);
		if (y > 7) y = 0;
//		co_memcpy(&devp->data, &netp->mac_address, 6);
	}
	co_debug("*********** autoconf:");
	dump_space(cmon);

	return CO_RC(OK);
}

void co_pci_request(co_monitor_t *cmon, int op) {

	switch(op) {
	case COPCI_GET_CONFIG:
		{
			copci_config_t *cp = (copci_config_t *) &co_passage_page->params[1];
			int x,y,count;

			/* Return config */
			count =0;
			for(x=0; x < COPCI_MAX_SLOTS; x++) {
				for(y=0; y < COPCI_MAX_FUNCS; y++) {
					if (cmon->pci_space[x][y].type) {
						cp->dev = x;
						cp->func = y;
						cp->type = cmon->pci_space[x][y].type;
						cp->unit = cmon->pci_space[x][y].unit;
						count++;
						cp++;
					}
				}
			}
			co_passage_page->params[0] = count;
		}
                break;
        default:
                co_passage_page->params[0] = 1;
                break;
        }

	return;
}
