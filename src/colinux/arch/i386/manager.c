/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <colinux/common/libc.h>

#include "manager.h"
#include "apic.h"

co_rc_t co_manager_arch_init(co_manager_t *manager, co_archdep_manager_t *out_archdep)
{
	co_rc_t rc;
	co_archdep_manager_t archdep;
	unsigned long cr4;

	*out_archdep = NULL;

	co_debug("arch init\n");
 
	archdep = (co_archdep_manager_t)co_os_malloc(sizeof(*archdep));
	co_memset(archdep, 0, sizeof(*archdep));	

	archdep->has_cpuid = co_i386_has_cpuid();
	if (archdep->has_cpuid == PFALSE) {
		co_debug("error, no CPUID!\n");
		rc = CO_RC(ERROR);
		goto out_error;
	}

	rc = co_i386_get_cpuid_capabilities(&archdep->caps);
	if (!CO_OK(rc)) {
		co_debug("error, couldn't get CPU capabilities\n");
		rc = CO_RC(ERROR);
		goto out_error;
	}

	asm("mov %%cr4, %0" : "=r"(cr4));

	if (cr4 & X86_CR4_PAE) {
		co_debug("PAE is enabled, cannot continue\n");
		rc = CO_RC(PAE_ENABLED);
		goto out_error;
	}

	rc = co_manager_arch_init_apic(archdep);
	if (!CO_OK(rc))
		goto out_error;

	*out_archdep = archdep;
	return rc;

out_error:
	co_os_free(archdep);
	return rc;
}

void co_manager_arch_free(co_archdep_manager_t archdep)
{
	co_debug("arch free\n");

	co_os_free(archdep);
}
