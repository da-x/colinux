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

#define DEBUG_FUNC co_debug
#define DEBUG_PRINT(format, params...) do {				\
		DEBUG_FUNC("%s():%d " format, __FUNCTION__, __LINE__, params); \
	} while (0);

#define DEBUG_PRINT_N(format) DEBUG_PRINT("%s", format);

#define KASSERT(condition)						\
	do { if (!(condition)) DEBUG_FUNC("ASSERT failed: %s\n", #condition); } while (0)

#define KVERIFY(condition) ({	         	\
     typeof(condition) _condition_ = condition;	\
     KASSERT(_condition_);		        \
     _condition_;                               \
})

#include <stdarg.h>

extern void co_debug_(const char *module, int level, const char *filename, int line, const char *fmt, ...);
extern void co_debug_system(const char *fmt, ...);

#define _colinux_attr_used __attribute__((__used__))

static char _colinux_attr_used colinux_file_id[] = __FILE__;
static char _colinux_attr_used *colinux_file_id_ptr = (char *)&colinux_file_id;
extern char _colinux_module[0x30];

#define COLINUX_DEFINE_MODULE(str) \
char _colinux_module[] = str;

#define co_debug_lvl(level, fmt, ...) \
	co_debug_(_colinux_module, level, colinux_file_id_ptr, __LINE__, fmt, ## __VA_ARGS__)

#define co_debug(fmt, ...) co_debug_lvl(10, fmt, ## __VA_ARGS__)

#else

/*------------------------------ Production Mode -----------------------------*/

#define KASSERT(condition)                   do {} while(0)
#define KVERIFY(condition)                   condition
#define DEBUG_PRINT                          do {} while(0)
#define DEBUG_PRINT_N                        do {} while(0)
#define co_debug(fmt, ...)                   do {} while(0)
#define co_debug_system(fmt, ...)            do {} while(0)

/*----------------------------------------------------------------------------*/

#endif

extern void co_debug_line(char *line);

#define co_debug_ulong(name) \
	co_debug("%s: 0x%x\n", #name, name)

#ifndef COLINUX_TRACE
#undef CO_TRACE_STOP
#define CO_TRACE_STOP
#undef CO_TRACE_CONTINUE
#define CO_TRACE_CONTINUE
#endif

#endif
