/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

/*
 * The passage page and code are responsible for switching between the 
 * two operating systems.
 */ 

#include <linux/config.h>
#include <colinux/common/common.h>
#include <colinux/kernel/monitor.h>
#include <colinux/arch/passage.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>

#include <memory.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/smp.h>

/* A hack to reduce Linux kernel headers inclusion */ 
#define __ASSEMBLY__
#include <asm/desc.h>

#include "cpuid.h"
#include "manager.h"

#ifdef __MINGW32__
#define SYMBOL_PREFIX "_"
#else
#define SYMBOL_PREFIX ""
#endif

/*
 * These two pseudo variables mark the start and the end of the passage code.
 * The passage code is position indepedent, so we just copy it from the 
 * driver's text to the passage page which we allocate when we start running.
 */

#define PASSAGE_CODE_WRAP_IBCS(_name_, inner)           \
extern char _name_;                                     \
extern char _name_##_end;                               \
							\
static inline unsigned long _name_##_size()		\
{							\
	return &_name_##_end - &_name_;			\
}							\
							\
static inline void memcpy_##_name_(void *dest)		\
{							\
	memcpy(dest, &_name_, _name_##_size());		\
}							\
							\
asm(""							\
    ".globl " SYMBOL_PREFIX #_name_          "\n"	\
    SYMBOL_PREFIX #_name_ ":"                "\n"	\
    "    push %ebx"                          "\n"	\
    "    push %esi"                          "\n"	\
    "    push %edi"                          "\n"	\
    "    push %ebp"                          "\n"	\
    inner						\
    "    popl %ebp"                          "\n"	\
    "    popl %edi"                          "\n"	\
    "    popl %esi"                          "\n"	\
    "    popl %ebx"                          "\n"	\
    "    ret"                                "\n"	\
    ".globl " SYMBOL_PREFIX #_name_ "_end"   "\n"	\
    SYMBOL_PREFIX #_name_ "_end:;"           "\n"	\
    "");

#define PASSAGE_CODE_NOWHERE_LAND()                     \
/* Turn off special processor features */               \
    "    movl %cr4, %ebx"                    "\n"       \
    "    andl $0xFFFFFF77, %ebx"             "\n"       \
    "    movl %ebx, %cr4"                    "\n"       \
/*
 *  Relocate to other map of the passage page.
 * 
 *  We do this by changing the current mapping to the passage temporary 
 *  address space, which contains the mapping of the passage page at the 
 *  two locations which are specific to the two operating systems.
 */							\
                                                        \
/* Put the virtual address of the source passage page in EBX */ \
    "    movl %ecx, %ebx"                    "\n"       \
    "    andl $0xFFFFF000, %ebx"             "\n"       \
                                                        \
/* 
 * Take the physical address of the temporary address space page directory 
 * pointer and put it in CR3.
 */ \
    "    movl (%ebx), %eax"                  "\n"       \
    "    movl %eax, %cr3"                    "\n"       \
/*
 * Read the 'other_map' field, which is the difference between the two
 * mappings.
 */ \
    "    movl "CO_ARCH_STATE_STACK_OTHERMAP"(%ebp), %eax"   "\n"  \
   \
/*  
 * First, we relocate EIP by putting it in 0x64(%ebp). That's why we load
 * ESP with 0x68(%esp). The call that follows puts the EIP where we want.
 * Afterwards the EIP is in %(esp) so we relocate it by adding the 
 * relocation offset. We also add the difference between 2 and 3 so that
 * the 'ret' that follows will put us in 3 intead of 2, but in the other 
 * mapping.
 */ \
    "    leal "CO_ARCH_STATE_STACK_RELOCATE_EIP_AFTER"(%ebp), %esp"              "\n"       \
    "    call 2f"                            "\n"       \
    "2:  addl %eax, (%esp)"                  "\n"       \
    "    addl $3f-2b, (%esp)"                "\n"       \
    "    ret"                                "\n"       \
    "3:  addl %eax, %ecx"                    "\n"       \
    "    movl %ecx, %ebp"                    "\n"       \


#define PASSAGE_CODE_WRAP_SWITCH(_inner_)				\
/* read return address and state pointers  */				\
    "    movl 16(%esp), %ebx" /* return addr */ "\n"			\
    "    movl 24(%esp), %ebp" /* current */     "\n"			\
    "    movl 28(%esp), %ecx" /* other */       "\n"			\
									\
/* save flags, disable interrupts */					\
    "    pushfl"                             "\n"			\
    "    cli"                                "\n"			\
									\
/* save and switch from old esp */					\
    "    movl %esp, "CO_ARCH_STATE_STACK_ESP"(%ebp)"   "\n"		\
    "    movl %ss, "CO_ARCH_STATE_STACK_SS"(%ebp)"   "\n"		\
									\
/* save flags */							\
    "    movl (%esp), %eax"                  "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_FLAGS"(%ebp)"    "\n"		\
									\
/* save return address */						\
    "    movl %ebx, "CO_ARCH_STATE_STACK_RETURN_EIP"(%ebp)"   "\n"	\
									\
/* save %cs */							        \  
    "    movl %cs, %ebx"                     "\n"			\
    "    movl %ebx, "CO_ARCH_STATE_STACK_CS"(%ebp)"      "\n"		\
									\
    _inner_								\
									\
/* get old ESP in EAX */						\
    "    lss "CO_ARCH_STATE_STACK_ESP"(%ebp), %eax" "\n"		\
									\
/* get return address */						\
    "    movl "CO_ARCH_STATE_STACK_RETURN_EIP"(%ebp), %ebx"  "\n"	\
    "    movl %ebx, 20(%eax)"                "\n"			\
									\
/* get flags */								\
    "    movl "CO_ARCH_STATE_STACK_FLAGS"(%ebp), %ebx"   "\n"		\
    "    movl %ebx, (%eax)"                  "\n"			\
									\
/* switch to old ESP */							\
    "    lss "CO_ARCH_STATE_STACK_ESP"(%ebp), %esp"   "\n"		\
									\
    "    call 1f"                            "\n"			\
    "1:  popl %eax"                          "\n"			\
    "    addl $2f-1b,%eax"                   "\n"			\
    "    movl 0x04(%ebp), %ebx"              "\n"			\
    "    movw %bx, -2(%eax)"                 "\n"			\
    "    movl %eax, -6(%eax)"                "\n"			\
    "    jmp 3f"                            "\n"			\
    "3:  ljmp $0,$0"                         "\n"			\
    "2:  popfl"                              "\n"

#define PASSAGE_PAGE_PRESERVATION_FXSAVE(_inner_)		\
    "    fxsave "CO_ARCH_STATE_STACK_FXSTATE"(%ebp)"  "\n"	\
    "    fnclex"                             "\n"		\
    _inner_							\
    "    fxrstor "CO_ARCH_STATE_STACK_FXSTATE"(%ebp)"   "\n"

#define PASSAGE_PAGE_PRESERVATION_FNSAVE(_inner_)		\
    "    fnsave "CO_ARCH_STATE_STACK_FXSTATE"(%ebp)" "\n"	\
    "    fwait"                              "\n"		\
    _inner_							\
    "    frstor "CO_ARCH_STATE_STACK_FXSTATE"(%ebp)" "\n"

#define PASSAGE_PAGE_PRESERVATION_DEBUG(_inner_)			\
/* Put the virtual address of the passage page in EBX */		\
    "    movl %ebp, %ebx"                    "\n"			\
    "    andl $0xFFFFF000, %ebx"             "\n"			\
/*  "    incb 0x20f(%ebx)"                   "\n" */			\
									\
/* save DR0 */								\
    "    movl %dr0, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR0"(%ebp)"              "\n"	\
    "    movl %eax, 0x4(%ebx)"               "\n"			\
									\
/* save DR1 */								\
    "    movl %dr1, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR1"(%ebp)"              "\n"	\
    "    movl %eax, 0x8(%ebx)"               "\n"			\
									\
/* save DR2 */								\
    "    movl %dr2, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR2"(%ebp)"              "\n"	\
    "    movl %eax, 0xc(%ebx)"               "\n"			\
									\
/* save DR3 */								\
    "    movl %dr3, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR3"(%ebp)"              "\n"	\
    "    movl %eax, 0x10(%ebx)"              "\n"			\
									\
/* save DR6 */								\
    "    movl %dr6, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR6"(%ebp)"              "\n"	\
    "    movl %eax, 0x14(%ebx)"              "\n"			\
									\
/* save DR7 */								\
    "    movl %dr7, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_DR7"(%ebp)"              "\n"	\
    "    movl $0x00000700, %eax"             "\n"			\
    "    movl %eax, 0x18(%ebx)"              "\n"			\
    "    movl %eax, %dr7"                    "\n"			\
									\
    _inner_								\
									\
/* Put the virtual address of the passage page in EBX */		\
    "    movl %ebp, %ebx"                    "\n"			\
    "    andl $0xFFFFF000, %ebx"             "\n"			\
									\
/* load DR0 */								\
    "    movl "CO_ARCH_STATE_STACK_DR0"(%ebp), %eax"    "\n"		\
    "    movl 0x4(%ebx), %ecx"               "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr0"                    "\n"			\
    "1:"                                     "\n"			\
									\
/* load DR1 */								\
    "    movl "CO_ARCH_STATE_STACK_DR1"(%ebp), %eax"    "\n"		\
    "    movl 0x8(%ebx), %ecx"               "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr1"                    "\n"			\
    "1:"                                     "\n"			\
									\
/* load DR2 */								\
    "    movl "CO_ARCH_STATE_STACK_DR2"(%ebp), %eax"    "\n"		\
    "    movl 0xC(%ebx), %ecx"               "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr2"                    "\n"			\
    "1:"                                     "\n"			\
									\
/* load DR3 */								\
    "    movl "CO_ARCH_STATE_STACK_DR3"(%ebp), %eax"    "\n"		\
    "    movl 0x10(%ebx), %ecx"              "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr3"                    "\n"			\
    "1:"                                     "\n"			\
									\
/* load DR6 */								\
    "    movl "CO_ARCH_STATE_STACK_DR6"(%ebp), %eax"    "\n"		\
    "    movl 0x14(%ebx), %ecx"              "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr6"                    "\n"			\
    "1:"                                     "\n"			\
									\
/* load DR7 */								\
    "    movl "CO_ARCH_STATE_STACK_DR7"(%ebp), %eax"    "\n"		\
    "    movl 0x18(%ebx), %ecx"              "\n"			\
    "    cmpl %eax, %ecx"                    "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl %eax, %dr7"                    "\n"			\
    "1:"                                     "\n"			\

#define PASSAGE_PAGE_PRESERVATION_COMMON(_inner_)			\
/* save GDT */								\
    "    leal "CO_ARCH_STATE_STACK_GDT"(%ebp), %ebx"        "\n"	\
    "    sgdt (%ebx)"                        "\n"			\
									\
/* save TR */								\
    "    xor %eax, %eax"                     "\n"			\
    "    str %ax"                            "\n"			\
    "    movw %ax, "CO_ARCH_STATE_STACK_TR"(%ebp)"          "\n"	\
									\
/*									\
 * If TR is not 0, turn off our task's BUSY bit so we don't get a GPF	\
 * on the way back.							\
 */									\
    "    cmpw $0, %ax"                       "\n"			\
    "    jz 1f"                              "\n"			\
    "    movl 2(%ebx), %edx"                 "\n"			\
    "    shr $3, %eax      "                 "\n"			\
    "    andl $0xfffffdff, 4(%edx,%eax,8)"   "\n"			\
    "1:"                                     "\n"			\
									\
/* save LDT */								\
    "    sldt "CO_ARCH_STATE_STACK_LDT"(%ebp)"                    "\n"	\
									\
/* save IDT */								\
    "    sidt "CO_ARCH_STATE_STACK_IDT"(%ebp)"                    "\n"	\
									\
/* save segment registers */						\
    "    movl %gs, %ebx"                     "\n"			\
    "    movl %ebx, "CO_ARCH_STATE_STACK_GS"(%ebp)"              "\n"	\
    "    movl %fs, %ebx"                     "\n"			\
    "    movl %ebx, "CO_ARCH_STATE_STACK_FS"(%ebp)"              "\n"	\
    "    movl %ds, %ebx"                     "\n"			\
    "    movl %ebx, "CO_ARCH_STATE_STACK_DS"(%ebp)"              "\n"	\
    "    movl %es, %ebx"                     "\n"			\
    "    movl %ebx, "CO_ARCH_STATE_STACK_ES"(%ebp)"              "\n"	\
									\
/* be on the safe side and nullify the segment registers */		\
    "    movl $0, %ebx"                      "\n"			\
    "    movl %ebx, %fs"                     "\n"			\
    "    movl %ebx, %gs"                     "\n"			\
									\
/* save CR4 */								\
    "    movl %cr4, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_CR4"(%ebp)"              "\n"	\
									\
/* save CR2 */								\
    "    movl %cr2, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_CR2"(%ebp)"              "\n"	\
									\
/* save CR0 */								\
    "    movl %cr0, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_CR0"(%ebp)"              "\n"	\
									\
/* save CR3 */								\
    "    movl %cr3, %eax"                    "\n"			\
    "    movl %eax, "CO_ARCH_STATE_STACK_CR3"(%ebp)"      "\n"		\
									\
_inner_									\
									\
/* load other's CR3 */							\
									\
    "    movl "CO_ARCH_STATE_STACK_CR3"(%ebp), %eax"    "\n"		\
    "    movl %eax, %cr3"                    "\n"			\
									\
/* load other's CR0 */							\
									\
    "    movl "CO_ARCH_STATE_STACK_CR0"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr0"                    "\n"			\
									\
/* load other's CR2 */							\
									\
    "    movl "CO_ARCH_STATE_STACK_CR2"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr2"                    "\n"			\
									\
/* load other's CR4 */							\
    "    movl "CO_ARCH_STATE_STACK_CR4"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr4"                    "\n"			\
									\
/* load other's GDT */							\
    "    lgdt "CO_ARCH_STATE_STACK_GDT"(%ebp)"                    "\n"	\
									\
/* load IDT */								\
    "    lidt "CO_ARCH_STATE_STACK_IDT"(%ebp)"                    "\n"	\
									\
/* load LDT */								\
    "    lldt "CO_ARCH_STATE_STACK_LDT"(%ebp)"                    "\n"	\
									\
/* load segment registers */						\
    "    movl "CO_ARCH_STATE_STACK_GS"(%ebp), %ebx"              "\n"	\
    "    movl %ebx, %gs"                     "\n"			\
    "    movl "CO_ARCH_STATE_STACK_FS"(%ebp), %ebx"              "\n"	\
    "    movl %ebx, %fs"                     "\n"			\
    "    movl "CO_ARCH_STATE_STACK_ES"(%ebp), %ebx"              "\n"	\
    "    movl %ebx, %es"                     "\n"			\
    "    movl "CO_ARCH_STATE_STACK_DS"(%ebp), %ebx"              "\n"	\
    "    movl %ebx, %ds"                     "\n"			\
    "    movl "CO_ARCH_STATE_STACK_SS"(%ebp), %ebx"              "\n"	\
    "    movl %ebx, %ss"                     "\n"			\
									\
/* load TR */								\
    "    movw "CO_ARCH_STATE_STACK_TR"(%ebp), %ax"               "\n"	\
    "    cmpw $0, %ax"                       "\n"			\
    "    jz 1f"                              "\n"			\
    "    ltr %ax"                            "\n"			\
    "1:"                                     "\n"

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func_fxsave, 
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_FXSAVE(	
			PASSAGE_PAGE_PRESERVATION_DEBUG(
				PASSAGE_PAGE_PRESERVATION_COMMON(
					PASSAGE_CODE_NOWHERE_LAND() 
					)
				)
			)
		)
	)

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func_fnsave,
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_FNSAVE(
			PASSAGE_PAGE_PRESERVATION_DEBUG(
				PASSAGE_PAGE_PRESERVATION_COMMON(
					PASSAGE_CODE_NOWHERE_LAND()
					)
				)
			)
		)
	)

co_rc_t co_monitor_arch_passage_page_alloc(co_monitor_t *cmon)
{
	co_rc_t rc;

	cmon->passage_page = co_os_alloc_pages(sizeof(co_arch_passage_page_t)/PAGE_SIZE);
	if (cmon->passage_page != NULL) {
		memset(cmon->passage_page, 0, sizeof(co_arch_passage_page_t));

		rc = CO_RC(OK);
	}
	else 
		rc = CO_RC(OUT_OF_MEMORY);

	return rc;
}

void co_monitor_arch_passage_page_free(co_monitor_t *cmon)
{
	co_os_free_pages(cmon->passage_page, sizeof(co_arch_passage_page_t)/PAGE_SIZE);
}

/*
 * Create the temp_pgd address space.
 *
 * The goal of this function is simple: Take the two virtual addresses
 * of the passage page from the operating systems and create an empty
 * address space which contains just these two virtual addresses, mapped
 * to the physical address of the passage page.
 */
void co_monitor_arch_passage_page_temp_pgd_init(co_arch_passage_page_t *pp, unsigned long *va)
{
	unsigned long i, pa = co_os_virt_to_phys(pp);
	struct {
		unsigned long pmd;
		unsigned long pte;
		unsigned long paddr;
	} maps[2];

	for (i=0; i < 2; i++) {
		maps[i].pmd = va[i] >> PMD_SHIFT;
		maps[i].pte = (va[i] & ~PMD_MASK) >> PAGE_SHIFT;
		maps[i].paddr = co_os_virt_to_phys(&pp->temp_pte[i]);
	}

	/*
	 * Currently this is not the case, but it is possible that 
	 * these two virtual address are on the same PTE page. In that
	 * case we don't need temp_pte[1].
	 *
	 * NOTE: If the two virtual addresses are identical then it
	 * would still work (other_map will be 0 in both contexts).
	 */

	if (maps[0].pmd == maps[1].pmd){
		/* Use one page table */

		pp->temp_pgd[maps[0].pmd] = _KERNPG_TABLE | maps[0].paddr;
		pp->temp_pte[0][maps[0].pte] = _KERNPG_TABLE | pa;
		pp->temp_pte[0][maps[1].pte] = _KERNPG_TABLE | pa;
	} else {
		/* Use two page tables */

		pp->temp_pgd[maps[0].pmd] = _KERNPG_TABLE | maps[0].paddr;
		pp->temp_pgd[maps[1].pmd] = _KERNPG_TABLE | maps[1].paddr;
		pp->temp_pte[0][maps[0].pte] = _KERNPG_TABLE | pa;
		pp->temp_pte[1][maps[1].pte] = _KERNPG_TABLE | pa;
	}
}

co_rc_t co_monitor_arch_passage_page_init(co_monitor_t *cmon)
{
	unsigned long va[2] = {
		(unsigned long)(cmon->passage_page_vaddr),
		(unsigned long)(cmon->passage_page),
	};
	co_arch_passage_page_t *pp = cmon->passage_page;
	pgd_t pgd;
	bool_t has_cpuid;
	co_rc_t rc;
	unsigned long caps = 0;

	caps = cmon->manager->archdep->caps;

	/*
	 * TODO: Add sysenter / sysexit restoration support 
	 */
	if (caps & (1 << X86_FEATURE_FXSR)) {
		co_debug("CPU supports fxsave/fxrstor\n");
		memcpy_co_monitor_passage_func_fxsave(&pp->code[0]);
	} else {
		co_debug("CPU supports fnsave/frstor\n");
		memcpy_co_monitor_passage_func_fnsave(&pp->code[0]);
	}
	
	/*
	 * We need to know the physical address of the temp_pgd page so 
	 * we would be able to set CR3 during the passage page code, thus
         * that physical address is saved inside the passage page itself.
	 */
	pp->temp_pgd_physical = co_os_virt_to_phys(&pp->temp_pgd);

	co_monitor_arch_passage_page_temp_pgd_init(pp, va);

	/*
	 * This "tests" the passage page by populating the Linux context with
	 * the state of the host OS. Later we use this as the base for 
	 * tuning the Linux context. 
	 * 
	 * Optionally it can be rewritten so wouldn't need it at all, and
	 * start with a completely _clean_ state.
	 */
	co_passage_page_func(linuxvm_state, linuxvm_state);

	co_debug("Passage page dump (Host context)\n");
	co_passage_page_dump(pp);

	/*
	 * Link the two states to each other so that the passage code properly
	 * relocates its EIP inside the temporary passage address space.
	 */
	pp->linuxvm_state.other_map = va[1] - va[0];
	pp->host_state = pp->linuxvm_state;
	pp->host_state.other_map = va[0] - va[1];

	/*
	 * Init the Linux context.
	 */ 
	pp->linuxvm_state.tr = 0;
	pp->linuxvm_state.ldt = 0;
	pp->linuxvm_state.cr4 &= ~(X86_CR4_MCE | X86_CR4_PGE | X86_CR4_OSXMMEXCPT);
	pgd = cmon->pgd;
	pp->linuxvm_state.cr3 = pgd_val(pgd);
	pp->linuxvm_state.gdt.base = (struct x86_dt_entry *)cmon->import.kernel_gdt_table;
	pp->linuxvm_state.gdt.limit = ((__TSS(NR_CPUS)) * 8) - 1;
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
	pp->linuxvm_state.flags &= ~(1 << 9); /* Turn IF off */
	pp->linuxvm_state.return_eip = cmon->import.kernel_colinux_start;
	pp->linuxvm_state.cs = __KERNEL_CS;
	pp->linuxvm_state.ds = __KERNEL_DS;
	pp->linuxvm_state.es = __KERNEL_DS;
	pp->linuxvm_state.fs = __KERNEL_DS;
	pp->linuxvm_state.gs = __KERNEL_DS;
	pp->linuxvm_state.ss = __KERNEL_DS;

	co_debug("Passage page dump: %x\n", co_monitor_arch_passage_page_init);
	co_debug("COUNTER: %d\n", pp->pad[0x20f]);

	co_passage_page_dump(pp);

	return CO_RC_OK;
}

void co_host_switch_wrapper(co_monitor_t *cmon)
{
	co_switch();
}
