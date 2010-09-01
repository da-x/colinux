/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "common.h"
#include "debug.h"

#include <colinux/common/libc.h>
#include <colinux/os/timer.h>
#include <stdarg.h>

CO_TRACE_STOP;

#ifdef COLINUX_TRACE
void co_trace_ent_name(void *ptr, const char *name)
{
	static int reenter = 0;

	if (reenter)
		return;

	reenter++;

	co_debug("TRACE: [%p] %s", ptr, name);

	reenter--;
}
#endif

#ifdef COLINUX_DEBUG
static inline co_debug_tlv_t *tlv_add_const_string(co_debug_type_t type, const char *str, co_debug_tlv_t *sub_tlv)
{
	sub_tlv->type = type;
	sub_tlv->length = co_strlen(str) + 1;
	co_memcpy(sub_tlv->value, str, sub_tlv->length);
	return (co_debug_tlv_t *)&sub_tlv->value[sub_tlv->length];
}

static inline co_debug_tlv_t *tlv_add_timestamp(co_debug_tlv_t *sub_tlv)
{
	sub_tlv->type = CO_DEBUG_TYPE_TIMESTAMP;
	sub_tlv->length = sizeof(co_timestamp_t);
	{
		co_timestamp_t *ts = (co_timestamp_t *)&sub_tlv->value;
		co_os_get_timestamp(ts);
	}
	return (co_debug_tlv_t *)&sub_tlv->value[sub_tlv->length];
}

static inline co_debug_tlv_t *tlv_add_unsigned_long(co_debug_type_t type, unsigned long data, co_debug_tlv_t *sub_tlv)
{
	sub_tlv->type = type;
	sub_tlv->length = sizeof(data);
 	*((typeof(data) *)sub_tlv->value) = data;
	return (co_debug_tlv_t *)&sub_tlv->value[sub_tlv->length];
}

static inline co_debug_tlv_t *tlv_add_char(co_debug_type_t type, unsigned long data, co_debug_tlv_t *sub_tlv)
{
	sub_tlv->type = type;
	sub_tlv->length = sizeof(data);
 	*((typeof(data) *)sub_tlv->value) = data;
	return (co_debug_tlv_t *)&sub_tlv->value[sub_tlv->length];
}

static int co_debug_local_index;
#define X(facility, static_level, default_dynamic_level) .facility##_level=default_dynamic_level,
co_debug_levels_t co_global_debug_levels = {
	CO_DEBUG_LIST
};
#undef X

void co_debug_(const char *module, co_debug_facility_t facility, int level,
	       const char *filename, int line, const char *func,
	       const char *fmt, ...)
{
	char buffer[512]; /* Typicaly headerlength app. 110 bytes */
	co_debug_tlv_t *sub_tlv = (co_debug_tlv_t *)buffer;
	va_list ap;

	sub_tlv = tlv_add_const_string(CO_DEBUG_TYPE_MODULE, module, sub_tlv);
	sub_tlv = tlv_add_const_string(CO_DEBUG_TYPE_FILE, filename, sub_tlv);
	sub_tlv = tlv_add_timestamp(sub_tlv);
	sub_tlv = tlv_add_unsigned_long(CO_DEBUG_TYPE_LOCAL_INDEX, ++co_debug_local_index, sub_tlv);
	sub_tlv = tlv_add_char(CO_DEBUG_TYPE_FACILITY, facility, sub_tlv);
	sub_tlv = tlv_add_const_string(CO_DEBUG_TYPE_FUNC, func, sub_tlv);
	sub_tlv = tlv_add_unsigned_long(CO_DEBUG_TYPE_LINE, line, sub_tlv);
	sub_tlv = tlv_add_char(CO_DEBUG_TYPE_LEVEL, level, sub_tlv);

	sub_tlv->type = CO_DEBUG_TYPE_STRING;

	va_start(ap, fmt);
	sub_tlv->length = co_vsnprintf(sub_tlv->value, sizeof(buffer) - (sub_tlv->value - buffer) - 1, fmt, ap) + 1;
	va_end(ap);

	co_debug_level_system(module, facility, level, filename, line, func, sub_tlv->value);
	co_debug_buf(buffer, sub_tlv->value + sub_tlv->length - buffer);
}
#endif

CO_TRACE_CONTINUE;

