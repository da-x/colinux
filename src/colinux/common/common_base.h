/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_COMMON_BASE_H__
#define __COLINUX_LINUX_COMMON_BASE_H__

typedef int bool_t;

#define PFALSE  0 
#define PTRUE   1

#define PACKED_STRUCT __attribute__((packed))

#define CO_MAX_MONITORS 16

#define CO_LINUX_PERIPHERY_API_VERSION    9

typedef enum {
    CO_RC_OK                         = 0,
    CO_RC_ERROR                      = -1,
    CO_RC_PAE_ENABLED                = -2,
    CO_RC_VERSION_MISMATCHED         = -3,
    CO_RC_OUT_OF_MEMORY              = -4,
    CO_RC_OUT_OF_PAGES               = -5,
    CO_RC_NOT_FOUND                  = -6,
    CO_RC_CANT_OPEN_VMLINUX_FILE     = -12,
    CO_RC_CANT_STAT_VMLINUX_FILE     = -13,
    CO_RC_ERROR_READING_VMLINUX_FILE = -14,
    CO_RC_ERROR_INSTALLING_DRIVER    = -15,
    CO_RC_ERROR_REMOVING_DRIVER      = -16,
    CO_RC_ERROR_STARTING_DRIVER      = -17,
    CO_RC_ERROR_STOPPING_DRIVER      = -18,
    CO_RC_ERROR_ACCESSING_DRIVER     = -19,
    CO_RC_TRANSFER_OFF_BOUNDS        = -20,
    CO_RC_MONITOR_NOT_LOADED         = -21,
    CO_RC_BROKEN_PIPE                = -22,
    CO_RC_TIMEOUT                    = -23,
    CO_RC_ACCESS_DENIED              = -24,
} co_rc_t;

#include "debug.h"

#define CO_RC_NEG_WRAPPER(name)   ((co_rc_t)((name < 0) ? (1 << 31 | -name | (__LINE__ << 12)) : name))
#define CO_RC_GET_CODE(name)      (-(name & 0xfff))
#define CO_RC_GET_LINE(name)      ((name >> 12) & 0x7ffff)

#ifndef DEBUG_CO_RC
#define CO_RC(name)      CO_RC_NEG_WRAPPER(CO_RC_##name)
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
 * Defines a LINUX instance. There are CO_MAX_COLINUXS of these 
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

#endif
