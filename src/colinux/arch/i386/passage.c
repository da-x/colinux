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

#include <colinux/common/debug.h>
#include <colinux/common/libc.h>
#include <colinux/common/common.h>
#include <colinux/kernel/monitor.h>
#include <colinux/arch/passage.h>
#include <colinux/os/kernel/alloc.h>
#include <colinux/os/kernel/misc.h>

#include "cpuid.h"
#include "manager.h"
#include "utils.h"
#include "antinx.h"
#include "defs.h"

#ifdef __MINGW32__
#define SYMBOL_PREFIX "_"
#else
#define SYMBOL_PREFIX ""
#endif

#ifdef CO_COLINUX_KERNEL
#error "CO_COLINUX_KERNEL should never defined here"
#endif

#ifdef CONFIG_COOPERATIVE
#error "CONFIG_COOPERATIVE should never defined here"
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
static inline unsigned long _name_##_size(void)		\
{							\
	return &_name_##_end - &_name_;			\
}							\
							\
static inline void memcpy_##_name_(void *dest)		\
{							\
	co_memcpy(dest, &_name_, _name_##_size());	\
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

#define PASSAGE_CODE_NOWHERE_LAND_SHORT()               \
    /* Turn off special processor features */           \
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

#define PASSAGE_CODE_NOWHERE_LAND()					\
    "    movl %ecx, %ebx"                    "\n"			\
    "    andl $0xFFFFF000, %ebx"             "\n"			\
    "    movl (%ebx), %edx"                  "\n"			\
    "    movl %edx, %eax"                    "\n"			\
    "    leal "CO_ARCH_STATE_STACK_RELOCATE_EIP_AFTER"(%ebp), %esp"           "\n" \
    "    call 2f"                            "\n"			\
    "2:  pop %ebx"                           "\n"			\
    "    andl $0x00000FFF, %ebx"             "\n"			\
    "    orl %eax, %ebx"                     "\n"			\
    "    addl $3f-2b, %ebx"                  "\n"			\
    "    movl "CO_ARCH_STATE_STACK_TEMP_CR3"(%ebp), %eax"  "\n"		\
    "    movl %eax, %cr3"                    "\n"			\
    "    call *%ebx"                         "\n"			\
    "    jmp 4f"                             "\n"			\
    "3:     "                                "\n"			\
    "    movl "CO_ARCH_STATE_STACK_VA"(%ebp), %eax" "\n"		\
    "    subl %edx, %eax"                    "\n"			\
    "    subl %eax, %ecx"                    "\n"			\
    "    subl %eax, %ebp"                    "\n"			\
    "    subl %eax, (%esp)"                  "\n"			\
    "    subl %eax, %esp"                    "\n"			\
									\
    /* bit31:PG=0 Disable Paging */						\
    "    movl %cr0, %eax"                    "\n"			\
    "    andl $0x7fffffff, %eax"             "\n"			\
    "    movl %eax, %cr0"                    "\n"			\
									\
    /* bit5:PAE=1 Enable PAE */						\
    "    movl "CO_ARCH_STATE_STACK_CR4"(%ecx), %eax"  "\n"		\
    "    andl $0x00000020, %eax"  "\n"					\
    "    movl %eax, %cr4" "\n"						\
    "    movl "CO_ARCH_STATE_STACK_TEMP_CR3"(%ecx), %eax"  "\n"		\
    "    movl %eax, %cr3" "\n"						\
									\
    /* bit31:PG=1 Enable Paging */						\
    "    movl %cr0, %eax"                    "\n"			\
    "    orl $0x80000000, %eax"              "\n"			\
    "    movl %eax, %cr0"                    "\n"			\
									\
    "    movl "CO_ARCH_STATE_STACK_VA"(%ecx), %eax" "\n"		\
    "    subl %edx, %eax"                    "\n"			\
    "    addl %eax, %esp"                    "\n"			\
    "    addl %eax, (%esp)"                  "\n"			\
    "    addl %eax, %ecx"                    "\n"			\
    "    movl %ecx, %ebp"                    "\n"			\
    "    ret"                                "\n"			\
									\
    "  4:\n"								\

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
    "    mov %ss, "CO_ARCH_STATE_STACK_SS"(%ebp)"      "\n"		\
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
    "    jmp 3f"                             "\n"			\
    "3:  ljmp $0,$0"                         "\n"			\
    "2:  popfl"                              "\n"

#define PASSAGE_PAGE_PRESERVATION_SYSENTER(_inner_)		\
    "    movl %ecx, %ebx" /* save ecx (other page) */	"\n"	\
    "    movl $"MSR_IA32_SYSENTER_CS", %ecx"		"\n"	\
    "    rdmsr"						"\n"	\
    "    movl %eax, "CO_ARCH_STATE_SYSENTER_CS"(%ebp)"	"\n"	\
    "    incl %ecx" /* MSR_IA32_SYSENTER_ESP */		"\n"	\
    "    rdmsr"						"\n"	\
    "    movl %eax, "CO_ARCH_STATE_SYSENTER_ESP"(%ebp)"	"\n"	\
    "    incl %ecx" /* MSR_IA32_SYSENTER_EIP */		"\n"	\
    "    rdmsr"						"\n"	\
    "    movl %eax, "CO_ARCH_STATE_SYSENTER_EIP"(%ebp)"	"\n"	\
    "    movl %ebx, %ecx" /* restore ecx */		"\n"	\
    _inner_							\
    "    xor %edx, %edx"				"\n"	\
    "    movl "CO_ARCH_STATE_SYSENTER_CS"(%ebp), %eax"	"\n"	\
    "    movl $"MSR_IA32_SYSENTER_CS", %ecx"		"\n"	\
    "    wrmsr"						"\n"	\
    "    movl "CO_ARCH_STATE_SYSENTER_ESP"(%ebp), %eax"	"\n"	\
    "    incl %ecx" /* MSR_IA32_SYSENTER_ESP */		"\n"	\
    "    wrmsr"						"\n"	\
    "    movl "CO_ARCH_STATE_SYSENTER_EIP"(%ebp), %eax"	"\n"	\
    "    incl %ecx" /* MSR_IA32_SYSENTER_EIP */		"\n"	\
    "    wrmsr"						"\n"	\

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
/* load other's CR4 */							\
    "    movl "CO_ARCH_STATE_STACK_CR4"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr4"                    "\n"			\
									\
/* load other's CR3 */							\
    "    movl "CO_ARCH_STATE_STACK_CR3"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr3"                    "\n"			\
									\
/* load other's CR0 */							\
    "    movl "CO_ARCH_STATE_STACK_CR0"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr0"                    "\n"			\
									\
/* load other's CR2 */							\
    "    movl "CO_ARCH_STATE_STACK_CR2"(%ebp), %eax"              "\n"	\
    "    movl %eax, %cr2"                    "\n"			\
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
    "1:"                                     "\n"                       \

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func_sysenter,
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_SYSENTER(
			PASSAGE_PAGE_PRESERVATION_DEBUG(
				PASSAGE_PAGE_PRESERVATION_COMMON(
					PASSAGE_CODE_NOWHERE_LAND()
					)
				)
			)
		)
	)

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func,
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_DEBUG(
			PASSAGE_PAGE_PRESERVATION_COMMON(
				PASSAGE_CODE_NOWHERE_LAND()
				)
			)
		)
	)

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func_short_sysenter,
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_SYSENTER(
			PASSAGE_PAGE_PRESERVATION_DEBUG(
				PASSAGE_PAGE_PRESERVATION_COMMON(
					PASSAGE_CODE_NOWHERE_LAND_SHORT()
					)
				)
			)
		)
	)

PASSAGE_CODE_WRAP_IBCS(
	co_monitor_passage_func_short,
	PASSAGE_CODE_WRAP_SWITCH(
		PASSAGE_PAGE_PRESERVATION_DEBUG(
			PASSAGE_PAGE_PRESERVATION_COMMON(
				PASSAGE_CODE_NOWHERE_LAND_SHORT()
				)
			)
		)
	)


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
	if (co_get_cr4() & CO_ARCH_X86_CR4_VMXE) {

		/*
		 * An other virtualization is running in VMX mode.
		 * coLinux does not cooperate with it.
		 * Abort the guest, but don't crash the host for now.
		 */

		co_passage_page->operation = CO_OPERATION_TERMINATE;
		co_passage_page->params[0] = CO_TERMINATE_VMXE;
		return;
	}

	co_switch();
}
