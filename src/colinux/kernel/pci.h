
#ifndef __CO_KERNEL_PCI_H__
#define __CO_KERNEL_PCI_H__

/*
 * This source code is a part of coLinux source package.
 *
 * Copyright (C) 2008 Steve Shoecraft <sshoecraft@earthlink.net>
 *
 * The code is licensed under the GPL.  See the COPYING file in
 * the root directory.
 *
 */

extern co_rc_t co_pci_setconfig(co_monitor_t *);
extern void co_pci_request(co_monitor_t *, int);

#endif
