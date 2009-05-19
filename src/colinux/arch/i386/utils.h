/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_I386_UTILS_H__
#define __COLINUX_ARCH_I386_UTILS_H__

#include <colinux/common/common.h>

extern bool_t co_is_pae_enabled(void);
extern unsigned long co_get_cr3(void);
extern unsigned long co_get_cr4(void);
extern unsigned long co_get_dr0(void);
extern unsigned long co_get_dr1(void);
extern unsigned long co_get_dr2(void);
extern unsigned long co_get_dr3(void);
extern unsigned long co_get_dr6(void);
extern unsigned long co_get_dr7(void);

#endif
