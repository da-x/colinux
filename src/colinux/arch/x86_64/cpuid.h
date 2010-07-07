/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#pragma once
#ifndef __COLINUX_ARCH_X86_64_CPUID_H__
#define __COLINUX_ARCH_X86_64_CPUID_H__

#include <colinux/common/common.h>

typedef union {
	struct {
		uint32_t eax, ebx, ecx, edx;
	};
	struct {
		uint32_t highest_op;
		char id_string[12];
	};
} cpuid_t;

bool_t co_i386_has_cpuid(void);
void co_i386_get_cpuid(uint32_t op, cpuid_t *cpuid);
co_rc_t co_i386_get_cpuid_capabilities(uint32_t *caps);

#endif
