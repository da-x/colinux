/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/common/common.h>

#include "utils.h"

bool_t co_is_pae_enabled(void)
{
	unsigned long cr4 = 0;
	asm("mov %%cr4, %0" : "=r"(cr4));
	return cr4 & 0x20;
}

unsigned long co_get_cr3(void)
{
	unsigned long cr3 = 0;
	asm("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}
