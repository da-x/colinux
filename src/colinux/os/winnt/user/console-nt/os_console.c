
/* WinNT host: low-level console routines */

/* TODO:
 * Move all low-level console functions in this file
 */

#include <windows.h>

#include "os_console.h"

/* Set cursor size 
   Parameters:
     curs_size_prc - size of cursor in percent
   Returns:
     >= 0 - norm
              old size of cursor in percent  
     <  0 - error
*/
int co_console_set_cursor_size(void* out_h, int curs_size_prc)
{
	int     res;
	
	res   = -1;
	if((HANDLE)out_h != INVALID_HANDLE_VALUE) {
		CONSOLE_CURSOR_INFO curs_info;
		
		GetConsoleCursorInfo((HANDLE)out_h, &curs_info);
		res = curs_info.dwSize;
		if(!curs_info.bVisible) {
			res = 0;
		}
		if(curs_size_prc <= 0) {
			curs_size_prc      = 0;
			curs_info.bVisible = FALSE;
			
		} else {
			if(curs_size_prc > 99) {
				curs_size_prc = 99;
			}
			curs_info.bVisible = TRUE;
		}
		curs_info.dwSize = curs_size_prc;
		SetConsoleCursorInfo((HANDLE)out_h, &curs_info);
	}
	return res;
} 
