/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_COMMON_H__
#define __COLINUX_LINUX_COMMON_H__

typedef int bool_t;

#define PFALSE  0 
#define PTRUE   1

#define PACKED_STRUCT __attribute__((packed))

#define CO_MAX_MONITORS        16
#define CO_MAX_BLOCK_DEVICES   32

typedef enum {
    CO_RC_OK               = 0,
    CO_RC_ERROR            = -1,
    CO_RC_INVALID_CO_ID    = -2,
    CO_RC_INVALID_COLX     = -3,
    CO_RC_OUT_OF_MEMORY    = -4,
    CO_RC_INVALID_SECTION_INDEX   = -5,
    CO_RC_SECTION_ALREADY_LOADED  = -6,
    CO_RC_SECTION_NOT_LOADED  = -7,
    CO_RC_NO_PGD=-8,
    CO_RC_NO_PTE=-9,
    CO_RC_NO_PFN=-10,
    CO_RC_NO_RPPTM=-11,
    CO_RC_CANT_OPEN_VMLINUX_FILE=-12,
    CO_RC_CANT_STAT_VMLINUX_FILE=-13,
    CO_RC_ERROR_READING_VMLINUX_FILE=-14,
    CO_RC_ERROR_INSTALLING_DRIVER=-15,
    CO_RC_ERROR_REMOVING_DRIVER=-16,
    CO_RC_ERROR_STARTING_DRIVER=-17,
    CO_RC_ERROR_STOPPING_DRIVER=-18,
    CO_RC_ERROR_ACCESSING_DRIVER=-19,
    CO_RC_TRANSFER_OFF_BOUNDS=-20,
    CO_RC_CMON_NOT_LOADED   = -21,
} co_rc_t;

#include "debug.h"

#ifndef DEBUG_CO_RC
#define CO_RC(name)      CO_RC_##name
#define CO_OK(rc)        (rc >= 0)
#else
#define CO_RC(name)      ({ \
                        co_rc_t __rc__ = CO_RC_##name; \
                        co_debug("%s():%d: CO_RC(%d)\n", __FUNCTION__, __LINE__, __rc__); \
                        __rc__; \
})
#define CO_OK(rc)        ({ \
			co_rc_t __rc__ = rc; \
                        co_debug("%s():%d: CO_OK(%d)\n", __FUNCTION__, __LINE__, __rc__); \
                        __rc__ >= 0; \
})
#endif

/*
 * Defines a COLX instance. There are CO_MAX_COLINUXS of these 
 */
typedef unsigned long co_id_t;

#define CO_INVALID_ID ((co_id_t)-1)

/*
 * The definition of a pathname on the host.
 */
typedef char co_pathname_t[0x100];

/*
 * The maximum number of sections that we can load off the core image.
 */
#define CO_MAX_CORE_SECTIONS                        0x100


/*
 * CO_HOST_API is defined when we compile code which interacts
 * with the API of the host OS. In the compilation mode, some types
 * are included by both Linux an the host OS (like, PAGE_SIZE), which
 * can create a problem if we include both of these definition in the
 * same header file. CO_HOST_API is here to avoid such collision form
 * happening by defining common types for both compilation modes.
 */

#ifdef CO_KERNEL
#ifdef CO_HOST_API
typedef unsigned long linux_pte_t;
typedef unsigned long linux_pmd_t;
typedef unsigned long linux_pgd_t;
#else

#include <asm/page.h>
typedef pte_t linux_pte_t;
typedef pmd_t linux_pmd_t;
typedef pgd_t linux_pgd_t;
#endif
#endif

#include <linux/cooperative.h>

#include <colinux/common/context.h>

#include "debug.h"

#ifndef NULL
#define NULL (void *)0
#endif

#endif
