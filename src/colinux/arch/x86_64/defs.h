/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * Based on definitions from Linux.
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_ARCH_I386_DEFS_H__
#define __COLINUX_ARCH_I386_DEFS_H__

/*
 * Some declarations copied from Linux.
 */

#define CO_ARCH_X86_FEATURE_APIC      (9)
#define CO_ARCH_X86_FEATURE_SEP       (11)
#define CO_ARCH_X86_FEATURE_FXSR      (24)

/* AMD cpuid feature bits */
#define CO_ARCH_AMD_FEATURE_NX        (20)


/*
 * Intel CPU features in CR4
 */
#define CO_ARCH_X86_CR4_VME		0x0001	/* enable vm86 extensions */
#define CO_ARCH_X86_CR4_PVI		0x0002	/* virtual interrupts flag enable */
#define CO_ARCH_X86_CR4_TSD		0x0004	/* disable time stamp at ipl 3 */
#define CO_ARCH_X86_CR4_DE		0x0008	/* enable debugging extensions */
#define CO_ARCH_X86_CR4_PSE		0x0010	/* enable page size extensions */
#define CO_ARCH_X86_CR4_PAE		0x0020	/* enable physical address extensions */
#define CO_ARCH_X86_CR4_MCE		0x0040	/* Machine check enable */
#define CO_ARCH_X86_CR4_PGE		0x0080	/* enable global pages */
#define CO_ARCH_X86_CR4_PCE		0x0100	/* enable performance counters at ipl 3 */
#define CO_ARCH_X86_CR4_OSFXSR		0x0200	/* enable fast FPU save and restore */
#define CO_ARCH_X86_CR4_OSXMMEXCPT	0x0400	/* enable unmasked SSE exceptions */
#define CO_ARCH_X86_CR4_VMXE		0x2000	/* enable VMX virtualization */

/*
 * Offsets for rdmsr/wrmsr
 */
#define MSR_IA32_SYSENTER_CS		"0x00000174"
#define MSR_IA32_SYSENTER_ESP		"0x00000175"
#define MSR_IA32_SYSENTER_EIP		"0x00000176"

#endif
