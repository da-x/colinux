
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
			if (cmon->config.pci[x][y].type == type && cmon->config.pci[x][y].unit == unit)
				return 1;
		}
	}
	return 0;
}

static void pci_space_add(co_monitor_t *cmon, int func, int type, int unit) {
	register int x;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		if (cmon->config.pci[x][0].type == 0 || cmon->config.pci[x][0].type == type) {
			if (pci_space_find(cmon, type, unit) == 0) {
				if (func && cmon->config.pci[x][0].type == 0) func = 0;
				co_debug("adding type %d to slot %d func %d", type, x, func);
				cmon->config.pci[x][func].type = type;
				cmon->config.pci[x][func].unit = unit;
			}
			break;
		}
	}
}

static void dump_space(co_monitor_t *cmon) {
	register int x,y;

	for(x=0; x < COPCI_MAX_SLOTS; x++) {
		for(y=0; y < COPCI_MAX_FUNCS; y++) {
			if (cmon->config.pci[x][y].type)
				co_debug("pci_space[%d][%d]: type: %d, unit: %d", x, y,
					cmon->config.pci[x][y].type, cmon->config.pci[x][y].unit);
		}
	}
}

co_rc_t co_pci_setconfig(co_monitor_t *cmon) {
	co_netdev_desc_t *netp;
	register int x,y,z;

	/* Fixup user settings */
	for(x=0; x < 32; x++) {
		for(y=z=0; y < 8; y++) {
			if (!cmon->config.pci[x][y].type) continue;
			if (y && cmon->config.pci[x][z].type == 0) {
				cmon->config.pci[x][z].type = cmon->config.pci[x][y].type;
				cmon->config.pci[x][z].unit = cmon->config.pci[x][y].unit;
				cmon->config.pci[x][y].type = cmon->config.pci[x][y].unit = 0;
				z++;
			}
		}
	}
	co_debug("*********** user settings:");
	dump_space(cmon);

#ifdef CONFIG_COOPERATIVE_VIDEO
	/* Video adapters */
	for(x=0; x < CO_MODULE_MAX_COVIDEO; x++) {
		if (cmon->video_devs[x]) {
			co_debug("adding video%d", x);
			pci_space_add(cmon, 0, CO_DEVICE_VIDEO, x);
		}
	}
#endif

#ifdef CONFIG_COOPERATIVE_AUDIO
	/* Audio adapters */
	for(x=0; x < CO_MODULE_MAX_COAUDIO; x++) {
		if (cmon->audio_devs[x]) {
			co_debug("adding audio%d", x);
			pci_space_add(cmon, 0, CO_DEVICE_AUDIO, x);
		}
	}
#endif

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
					if (cmon->config.pci[x][y].type) {
						cp->dev = x;
						cp->func = y;
						cp->type = cmon->config.pci[x][y].type;
						cp->unit = cmon->config.pci[x][y].unit;
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
