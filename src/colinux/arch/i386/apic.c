/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 * This code is based on Linux 2.6.x arch/i386/kernel/apic.c.
 */

#include <colinux/common/common.h>

#include "defs.h"
#include "apic.h"

co_rc_t co_manager_arch_init_apic(co_archdep_manager_t manager)
{
	co_debug("arch APIC initialization");

	if (!(manager->caps[0] & CO_ARCH_X86_FEATURE_APIC)) {
		co_debug_error("no APIC support");
		return CO_RC(ERROR);
	}

	co_debug("found APIC support");



	/*
	 * BIG TODO:
	 *
	 * Find if we really need to mangle with APIC in coLinux.
	 * The SMP problem could be unrelated to APIC.
	 */

	return CO_RC(OK);
}
