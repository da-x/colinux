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

#define X(name) "CO_RC_ERROR_"#name,
static const char *co_errors_strings[] = {
	"<no error>",
	CO_ERRORS_X_MACRO
};
#undef X

void co_rc_format_error(co_rc_t rc, char *buf, int size)
{
	int code;

	if (CO_OK(rc)) {
		co_snprintf(buf, size, "success - line %d, file id %d",
			 CO_RC_GET_LINE(rc),
			 CO_RC_GET_FILE_ID(rc));
	} else {
		const char *code_string;
		int file_id;

		code = -CO_RC_GET_CODE(rc);
		if (code < (sizeof(co_errors_strings)/sizeof(co_errors_strings[0]))) {
			code_string = co_errors_strings[code];
		} else {
			code_string = "<no error>";
		}

		file_id = CO_RC_GET_FILE_ID(rc);
		
		co_snprintf(buf, size, "error - %s, line %d, file %s (%d)",
			    code_string, CO_RC_GET_LINE(rc), colinux_obj_filenames[file_id], file_id);
	}
}

