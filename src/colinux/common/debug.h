/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@gmx.net>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_DEBUG_H__
#define __COLINUX_DEBUG_H__

#define CO_DEBUG

#ifdef CO_DEBUG

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

#else

#define KASSERT(condition)   do {} while(0)
#define KVERIFY(condition)   condition
#define DEBUG_PRINT          do {} while(0)
#define DEBUG_PRINT_N        do {} while(0)

#endif

#include <stdarg.h>

extern void co_debug(const char *fmt, ...);
extern void co_vdebug(const char *format, va_list ap);
extern void co_debug_line(char *line);
extern void co_snprintf(char *buf, int size, const char *format, ...);

#ifndef COLINUX_TRACE
#undef CO_TRACE_STOP
#define CO_TRACE_STOP
#undef CO_TRACE_CONTINUE
#define CO_TRACE_CONTINUE
#endif

#endif
