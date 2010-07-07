/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#include <colinux/common/common.h>

#include "utils.h"

uint64_t co_get_cr4(void)
{
	uint64_t cr4 = 0;
	asm("mov %%cr4, %0" : "=r"(cr4));
	return cr4;
}

bool_t co_is_pae_enabled(void)
{
	return co_get_cr4() & 0x20;
}

uint64_t co_get_cr3(void)
{
	uint64_t cr3 = 0;
	asm("mov %%cr3, %0" : "=r"(cr3));
	return cr3;
}

uint64_t co_get_dr0(void)
{
	uint64_t reg = 0;
	asm("mov %%dr0, %0" : "=r"(reg));
	return reg;
}

uint64_t co_get_dr1(void)
{
	uint64_t reg = 0;
	asm("mov %%dr1, %0" : "=r"(reg));
	return reg;
}

uint64_t co_get_dr2(void)
{
	uint64_t reg = 0;
	asm("mov %%dr2, %0" : "=r"(reg));
	return reg;
}

uint64_t co_get_dr3(void)
{
	uint64_t reg = 0;
	asm("mov %%dr3, %0" : "=r"(reg));
	return reg;
}

uint64_t co_get_dr6(void)
{
	uint64_t reg = 0;
	asm("mov %%dr6, %0" : "=r"(reg));
	return reg;
}

uint64_t co_get_dr7(void)
{
	uint64_t reg = 0;
	asm("mov %%dr7, %0" : "=r"(reg));
	return reg;
}
