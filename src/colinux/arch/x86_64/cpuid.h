/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_I386_CPUID_H__
#define __COLINUX_ARCH_I386_CPUID_H__

#include <colinux/common/common.h>

typedef union {
	struct {
		unsigned long eax, ebx, ecx, edx;
	};
	struct {
		unsigned long highest_op;
		char id_string[12];
	};
} cpuid_t;

bool_t co_i386_has_cpuid(void);
void co_i386_get_cpuid(unsigned long op, cpuid_t *cpuid);
co_rc_t co_i386_get_cpuid_capabilities(unsigned long *caps);

#endif
