/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 * Copied some stuff over from Linux's arch/i386.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "cpuid.h"

bool_t co_i386_has_cpuid()
{
	bool_t has_cpuid = 0;
        /*
	 * This function assumes at least $86.
	 *
	 * It's good enough...
	 */

	asm(
		/* FIXME: W64: Check, is this code working? */
		/* Try change the CPUID FLAGS bit (put the old FLAGS in ECX) */
		"pushf\n"
		"pop %%rax\n"
		"mov %%rax,%%rcx\n"
		"xor %1,%%rax\n"
		"push %%rax\n"
		"popf\n"

		/* Read the new FLAGS (and restore the FLAGS from ECX) */
		"pushf\n"
		"pop %%rax\n"
		"xor %%rcx,%%rax\n"
		"push %%rcx\n"
		"popf\n"

		/* Was the bit changed? */
		"movl %3, %0\n"
		"andl %1,%%eax\n"
		"jz 1f\n"
		"movl %2, %0\n"
		"1:\n"
		: "=m"(has_cpuid)
		: "i"(0x200000), "i" (PTRUE), "i"(PFALSE)
		: "eax", "ecx"
		);

	return has_cpuid;
}

void co_i386_get_cpuid(uint32_t op, cpuid_t *cpuid)
{
	asm("cpuid"
	    : "=a" (cpuid->eax),
	      "=b" (cpuid->ebx),
	      "=c" (cpuid->ecx),
	      "=d" (cpuid->edx)
	    : "0" (op));
}

co_rc_t co_i386_get_cpuid_capabilities(uint32_t *caps)
{
	cpuid_t cpuid;
	uint32_t highest_op;

	co_i386_get_cpuid(0, &cpuid);
	highest_op = cpuid.highest_op;
	if (highest_op < 0x00000001) {
		/* What's wrong with this CPU?! */
		return CO_RC(ERROR);
	}

	co_i386_get_cpuid(0x00000001, &cpuid);

	caps[0] = cpuid.edx;

	/* Inspired by Linux's arch/i386/kernel/cpu/common.c: */
	co_i386_get_cpuid(0x80000000, &cpuid);

	/* Get the AMD caps: */
	if ((cpuid.eax & 0xffff0000) == 0x80000000) {
		if (cpuid.eax >= 0x80000001) {
			co_i386_get_cpuid(0x80000001, &cpuid);
			caps[1] = cpuid.edx;
		}
	}

	return CO_RC(OK);
}
