/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_DEBUG_H__
#define __COLINUX_DEBUG_H__

#ifdef COLINUX_DEBUG

/*-------------------------------- Debug Mode --------------------------------*/

#include <stdarg.h>

extern char _colinux_module[0x30];
extern const char *colinux_obj_filenames[];

#define COLINUX_DEFINE_MODULE(str) \
char _colinux_module[] = str;

#define CO_DEBUG_LIST \
	X(misc,           15, 0) \
	X(network,        15, 0) \
	X(messages,       15, 0) \
	X(prints,         15, 0) \
	X(blockdev,       15, 0) \
	X(allocations,    15, 0) \
	X(context_switch, 15, 0) \
	X(pipe,           15, 0) \
	X(filesystem,     15, 0) \

typedef struct {
#define X(facility, static_level, default_dynamic_level) int facility##_level;
	CO_DEBUG_LIST
#undef X
} co_debug_levels_t;

typedef enum {
#define X(facility, static_level, default_dynamic_level) CO_DEBUG_FACILITY_##facility,
	CO_DEBUG_LIST
#undef X
} co_debug_facility_t;

extern void co_debug_startup(void);
extern void co_debug_system(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
extern void co_debug_level_system(const char *module, co_debug_facility_t facility, int level,
			const char *filename, int line, const char *func, const char *text);

extern void co_debug_(const char *module, co_debug_facility_t facility, int level,
		      const char *filename, int line, const char *func,
		      const char *fmt, ...)
	__attribute__ ((format (printf, 7, 8)));

extern co_debug_levels_t co_global_debug_levels;

#define X(facility, static_level, default_dynamic_level)	        \
static inline int co_debug_log_##facility##_level(int requested_level)  \
{									\
	if (requested_level > static_level)				\
		return 0;						\
	return (requested_level <= co_global_debug_levels.facility##_level);      \
}
CO_DEBUG_LIST;
#undef X

#define co_debug_lvl(facility, __level__, fmt, ...) do { \
        if (co_debug_log_##facility##_level(__level__))  \
		co_debug_(_colinux_module, CO_DEBUG_FACILITY_##facility, __level__, __FILE__, \
                          __LINE__, __FUNCTION__, fmt, ## __VA_ARGS__);	\
} while (0)

#else

/*------------------------------ Production Mode -----------------------------*/

#define co_debug_startup()                         do {} while(0)
#define co_debug_lvl(facility, level, fmt, ...)    do {} while(0)
#define co_debug_system(fmt, ...)                  do {} while(0)

/*----------------------------------------------------------------------------*/

#define COLINUX_DEFINE_MODULE(str) /* str */

#endif

typedef enum {
	CO_DEBUG_TYPE_TLV,
	CO_DEBUG_TYPE_TIMESTAMP,
	CO_DEBUG_TYPE_STRING,
	CO_DEBUG_TYPE_MODULE,
	CO_DEBUG_TYPE_LOCAL_INDEX,
	CO_DEBUG_TYPE_FACILITY,
	CO_DEBUG_TYPE_LEVEL,
	CO_DEBUG_TYPE_LINE,
	CO_DEBUG_TYPE_FUNC,
	CO_DEBUG_TYPE_FILE,
	CO_DEBUG_TYPE_DRIVER_INDEX,
} co_debug_type_t;

typedef struct {
	char type;
	unsigned char length;
	char value[];
} co_debug_tlv_t;

extern void co_debug_buf(const char *buf, long size);

#define co_debug_ulong(name)     co_debug("%s: 0x%lx", #name, name)
#define co_debug(fmt, ...)       co_debug_lvl(misc, 10, fmt, ## __VA_ARGS__)
#define co_debug_error(fmt, ...) co_debug_lvl(misc, 3, fmt, ## __VA_ARGS__)
#define co_debug_info(fmt, ...)  co_debug_lvl(misc, 1, fmt, ## __VA_ARGS__)

#ifndef COLINUX_TRACE
#undef CO_TRACE_STOP
#define CO_TRACE_STOP
#undef CO_TRACE_CONTINUE
#define CO_TRACE_CONTINUE
#endif

#endif
