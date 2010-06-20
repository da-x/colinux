/*
 * This source code is a part of coLinux source package.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/*
 * The passage page and code are responsible for switching between the 
 * two operating systems.
 *
 * FIXME: W64: This code is not supoorted yet. It is a placeholder only.
 */

#include <colinux/common/debug.h>
#include <colinux/common/libc.h>
#include <colinux/common/common.h>
#include <colinux/common/common64.h>
#include <colinux/kernel/monitor.h>
#include <colinux/arch/passage.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>

#include "cpuid.h"
#include "manager.h"
#include "utils.h"
#include "antinx.h"
#include "defs.h"


co_rc_t co_monitor_arch_passage_page_alloc(co_monitor_t *cmon)
{
	co_rc_t rc;

	cmon->archdep = co_os_malloc(sizeof(*cmon->archdep));
	if (cmon->archdep == NULL)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(cmon->archdep, 0, sizeof(*cmon->archdep));

	cmon->passage_page = co_os_alloc_pages(sizeof(co_arch_passage_page_t)/CO_ARCH_PAGE_SIZE);
	if (cmon->passage_page == NULL){
		rc = CO_RC(OUT_OF_MEMORY);
		goto error;
	}

	co_memset(cmon->passage_page, 0, sizeof(co_arch_passage_page_t));

	rc = co_arch_anti_nx_init(cmon);
	if (!CO_OK(rc))
		goto error;

	return rc;

error:
	co_monitor_arch_passage_page_free(cmon);
	return rc;
}

void co_monitor_arch_passage_page_free(co_monitor_t *cmon)
{
	if (cmon->archdep) {
		co_arch_anti_nx_free(cmon);
		co_os_free(cmon->archdep);
		cmon->archdep = NULL;
	}
	if (cmon->passage_page) {
		co_os_free_pages(cmon->passage_page, sizeof(co_arch_passage_page_t)/CO_ARCH_PAGE_SIZE);
		cmon->passage_page = NULL;
	}
}

static inline void co_passage_page_dump_state(co_arch_state_stack_t *state)
{
	co_debug("cs: %04lx   ds: %04lx   es: %04lx   fs: %04lx   gs: %04lx   ss: %04lx",
		 state->cs, state->ds, state->es, state->fs, state->gs, state->ss);

	co_debug("cr0: %08lx   cr2: %08lx   cr3: %08lx   cr4: %08lx",
		 state->cr0, state->cr2, state->cr3, state->cr4);

	co_debug("dr0: %08lx   dr1: %08lx  dr2: %08lx  dr3: %08lx  dr6: %08lx  dr7: %08lx",
		 state->dr0, state->dr1, state->dr2, state->dr3, state->dr6, state->dr7);

	co_debug("gdt: %08lx:%04x   idt:%08lx:%04x   ldt:%04x  tr:%04x",
		 (long)state->gdt.base, state->gdt.limit,
		 (long)state->idt.table, state->idt.size,
		 state->ldt, state->tr);

	co_debug("return_eip: %08lx   flags: %08lx   esp: %08lx",
		 state->return_eip, state->flags, state->esp);

	co_debug("sysenter cs: %08lx    eip: %08lx   esp: %08lx",
		 state->sysenter_cs, state->sysenter_eip, state->sysenter_esp);
}

static inline void co_passage_page_dump(co_arch_passage_page_t *page)
{
	co_debug("Host state");
	co_passage_page_dump_state(&page->host_state);

	co_debug("Linux state");
	co_passage_page_dump_state(&page->linuxvm_state);
}

static void short_temp_address_space_init(co_arch_passage_page_normal_address_space_t *pp,
					  unsigned long pa, unsigned long *va)
{
	int i;

	struct {
		unsigned long pmd;
		unsigned long pte;
		unsigned long paddr;
	} maps[2];

	for (i=0; i < 2; i++) {
		maps[i].pmd = va[i] >> CO_ARCH_PMD_SHIFT;
		maps[i].pte = (va[i] & ~CO_ARCH_PMD_MASK) >> CO_ARCH_PAGE_SHIFT;
		maps[i].paddr = co_os_virt_to_phys(&pp->pte[i]);
	}
	/*
	 * Currently this is not the case, but it is possible that
	 * these two virtual address are on the same PTE page. In that
	 * case we don't need pte[1].
	 *
	 * NOTE: If the two virtual addresses are identical then it
	 * would still work (other_map will be 0 in both contexts).
	 */

	if (maps[0].pmd == maps[1].pmd){
		/* Use one page table */

		pp->pgd[maps[0].pmd] = _KERNPG_TABLE | maps[0].paddr;
		pp->pte[0][maps[0].pte] = _KERNPG_TABLE | pa;
		pp->pte[0][maps[1].pte] = _KERNPG_TABLE | pa;
	} else {
		/* Use two page tables */

		pp->pgd[maps[0].pmd] = _KERNPG_TABLE | maps[0].paddr;
		pp->pgd[maps[1].pmd] = _KERNPG_TABLE | maps[1].paddr;
		pp->pte[0][maps[0].pte] = _KERNPG_TABLE | pa;
		pp->pte[1][maps[1].pte] = _KERNPG_TABLE | pa;
	}
}


/*
 * Create the temp_pgd address space.
 *
 * The goal of this function is simple: Take the two virtual addresses
 * of the passage page from the operating systems and create an empty
 * address space which contains just these two virtual addresses, mapped
 * to the physical address of the passage page.
 */
static void normal_temp_address_space_init(co_arch_passage_page_normal_address_space_t *as,
					   unsigned long pa, unsigned long va)
{
	int i;
	unsigned long vas[2] = {
		(unsigned long)(va),
		(unsigned long)(pa),
	};

	for (i=0; i < 2; i++) {
		unsigned long pmd = vas[i] >> CO_ARCH_PMD_SHIFT;
		unsigned long pte = (vas[i] & ~CO_ARCH_PMD_MASK) >> CO_ARCH_PAGE_SHIFT;
		int j = i;

		if (!as->pgd[pmd]) {
			as->pgd[pmd] = _KERNPG_TABLE | co_os_virt_to_phys(&as->pte[j]);
		} else {
			j = 0;
		}

		as->pte[j][pte] = _KERNPG_TABLE | pa;
	}
}

static void pae_temp_address_space_init(co_arch_passage_page_pae_address_space_t *as,
					unsigned long pa, unsigned long va)
{
	int i;
	unsigned long vas[2] = {
		(unsigned long)(va),
		(unsigned long)(pa),
	};

	for (i=0; i < 2; i++) {
		unsigned long pgd = vas[i] >> CO_ARCH_PAE_PGD_SHIFT;
		unsigned long pmd = (vas[i] & ~CO_ARCH_PAE_PGD_MASK) >> CO_ARCH_PAE_PMD_SHIFT;
		unsigned long pte = (vas[i] & ~CO_ARCH_PAE_PMD_MASK) >> CO_ARCH_PAGE_SHIFT;
		int j = i, k = i;

		if (!as->main[pgd]) {
			as->main[pgd] = 0x1 | co_os_virt_to_phys(&as->pgd[j]);
		} else {
			j = 0;
		}

		if (!as->pgd[j][pmd]) {
			as->pgd[j][pmd] = _KERNPG_TABLE | co_os_virt_to_phys(&as->pte[k]);
		} else {
			k = 0;
		}

		as->pte[k][pte] = _KERNPG_TABLE | pa;
	}
}


co_rc_t co_monitor_arch_passage_page_init(co_monitor_t *cmon)
{
	co_arch_passage_page_t *pp = cmon->passage_page;
	unsigned long caps;

/* ...
	if (co_monitor_passage_func_short_sysenter_size() > sizeof (pp->code))
		return CO_RC(ERROR);
	if (co_monitor_passage_func_sysenter_size() > sizeof (pp->code))
		return CO_RC(ERROR);
	if (co_monitor_passage_func_short_size() > sizeof (pp->code))
		return CO_RC(ERROR);
	if (co_monitor_passage_func_size() > sizeof (pp->code))
		return CO_RC(ERROR);

	caps = cmon->manager->archdep->caps[0];

	if (caps & (1 << CO_ARCH_X86_FEATURE_SEP)) {
		co_debug("CPU supports sysenter/sysexit");
		if (!co_is_pae_enabled()) {
			memcpy_co_monitor_passage_func_short_sysenter(pp->code);
		} else {
			memcpy_co_monitor_passage_func_sysenter(pp->code);
		}
	} else {
		if (!co_is_pae_enabled()) {
			memcpy_co_monitor_passage_func_short(pp->code);
		} else {
			memcpy_co_monitor_passage_func(pp->code);
		}
	}
... */

	pp->self_physical_address = co_os_virt_to_phys(&pp->first_page);

	/*
	 * Init temporary address space page tables for host side:
	 */
	if (!co_is_pae_enabled()) {
		unsigned long va[2] = {
			(unsigned long)(cmon->passage_page_vaddr),
			(unsigned long)(pp),
		};

		pp->temp_pgd_physical = co_os_virt_to_phys(&pp->temp_space.pgd);
		short_temp_address_space_init(&pp->temp_space, co_os_virt_to_phys(&pp->first_page), va);

		pp->linuxvm_state.other_map = va[1] - va[0];
		pp->host_state.other_map = va[0] - va[1];

	} else {
		pae_temp_address_space_init(&pp->host_pae, pp->self_physical_address,
					    (unsigned long)(&pp->first_page));
		pp->host_state.temp_cr3 = co_os_virt_to_phys(&pp->host_pae);
		pp->host_state.va = (unsigned long)(&pp->first_page);

		/*
		 * Init the Linux context.
		 */
		pp->linuxvm_state.temp_cr3 = co_os_virt_to_phys(&pp->guest_normal);
		normal_temp_address_space_init(&pp->guest_normal, pp->self_physical_address,
					       (unsigned long)(cmon->passage_page_vaddr));

		pp->linuxvm_state.va = (unsigned long)(cmon->passage_page_vaddr);
	}

	pp->linuxvm_state.dr0 = co_get_dr0();
	pp->linuxvm_state.dr1 = co_get_dr1();
	pp->linuxvm_state.dr2 = co_get_dr2();
	pp->linuxvm_state.dr3 = co_get_dr3();
	pp->linuxvm_state.dr6 = co_get_dr6();
	pp->linuxvm_state.dr7 = co_get_dr7();
	pp->linuxvm_state.tr = 0;
	pp->linuxvm_state.ldt = 0;
	pp->linuxvm_state.cr4 = 0;
	pp->linuxvm_state.cr3 = cmon->pgd;
	pp->linuxvm_state.gdt.base = (struct x86_dt_entry *)cmon->import.kernel_gdt_table;
	pp->linuxvm_state.gdt.limit = (8*0x20) - 1;
	pp->linuxvm_state.idt.table = (struct x86_idt_entry *)cmon->import.kernel_idt_table;
	pp->linuxvm_state.idt.size = 256*8 - 1;
	pp->linuxvm_state.cr0 = 0x80010031;

	if (pp->linuxvm_state.gdt.limit < pp->host_state.gdt.limit)
		pp->linuxvm_state.gdt.limit = pp->host_state.gdt.limit;

	/*
	 * The stack doesn't start right in 0x2000 because we need some slack for
	 * the exit of the passage code.
	 */
	pp->linuxvm_state.esp = cmon->import.kernel_init_task_union + 0x2000 - 0x50;
	pp->linuxvm_state.flags = 0;
	pp->linuxvm_state.return_eip = cmon->import.kernel_colinux_start;

	pp->linuxvm_state.cs = cmon->arch_info.kernel_cs;
	pp->linuxvm_state.ds = \
	pp->linuxvm_state.es = \
	pp->linuxvm_state.fs = \
	pp->linuxvm_state.gs = \
	pp->linuxvm_state.ss = cmon->arch_info.kernel_ds;

	co_passage_page_dump(pp);

	return CO_RC_OK;
}

void co_host_switch_wrapper(co_monitor_t *cmon)
{
	// FIXME: W64: Prevent stupid users from running here!

	co_passage_page->operation = CO_OPERATION_TERMINATE;
	co_passage_page->params[0] = CO_TERMINATE_NOT_SUPPORTED;
	return;

	//co_switch();
}
