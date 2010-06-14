
/* WinNT host: low-level console routines */

/* TODO:
 * Move all low-level console functions in this file
 */

#include <windows.h>
#include <colinux/common/console.h>

#include "os_console.h"

/* Set cursor size
   Parameters:
     cursor_type - size of cursor in Linux kernel defines
*/
void co_console_set_cursor_size(void* out_h, const int cursor_type)
{
	CONSOLE_CURSOR_INFO curs_info;

	if((HANDLE)out_h == INVALID_HANDLE_VALUE)
		return;

	curs_info.bVisible = TRUE;
	switch (cursor_type) {
		case CO_CUR_NONE:
			curs_info.dwSize = 1;
			curs_info.bVisible = FALSE;
			break;
		case CO_CUR_UNDERLINE:
			curs_info.dwSize = 16;
			break;
		case CO_CUR_LOWER_THIRD:
			curs_info.dwSize = 33;
			break;
		case CO_CUR_LOWER_HALF:
			curs_info.dwSize = 50;
			break;
		case CO_CUR_TWO_THIRDS:
			curs_info.dwSize = 66;
			break;
		case CO_CUR_BLOCK:
			curs_info.dwSize = 99;
			break;
		case CO_CUR_DEF:
		default:
			curs_info.dwSize = 10;  /* This will never use in normal way */
	}

	SetConsoleCursorInfo((HANDLE)out_h, &curs_info);
}
