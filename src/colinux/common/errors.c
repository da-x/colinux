/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include "common.h"
#include "debug.h"

#ifdef COLINUX_DEBUG
static int file_id_max;
#endif

#define X(name) "CO_RC_ERROR_"#name,
static const char *co_errors_strings[] = {
	"<no error>",
	CO_ERRORS_X_MACRO
};
#undef X

void co_rc_format_error(co_rc_t rc, char *buf, int size)
{
	if (CO_OK(rc)) {
		co_snprintf(buf, size, "success - line %ld, file id %ld",
			 CO_RC_GET_LINE(rc),
			 CO_RC_GET_FILE_ID(rc));
	} else {
		unsigned int code;
		unsigned int file_id;
		const char *code_string;
		const char *file_string;

		code = -CO_RC_GET_CODE(rc);
		if (code < (sizeof(co_errors_strings)/sizeof(co_errors_strings[0]))) {
			code_string = co_errors_strings[code];
		} else {
			code_string = "<unknown error>";
		}

#ifdef COLINUX_DEBUG
		if (file_id_max == 0) {
			while (colinux_obj_filenames[file_id_max])
				file_id_max++;
		}

		file_id = CO_RC_GET_FILE_ID(rc);
		if (file_id < file_id_max) {
			file_string = colinux_obj_filenames[file_id];
		} else {
			file_string = "<unknown file>";
		}
#else
		file_id = 0;
		file_string = "<unknown file>";
#endif

		co_snprintf(buf, size, "error - %s, line %ld, file %s (%d)",
			    code_string, CO_RC_GET_LINE(rc), file_string, file_id);
	}
}

