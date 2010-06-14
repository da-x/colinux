
/* WinNT host: low level console routines */

#ifndef __CO_OS_USER_WINNT_OS_CONSOLE_H__
#define __CO_OS_USER_WINNT_OS_CONSOLE_H__

#if defined __cplusplus
extern "C" {
#endif

void co_console_set_cursor_size(void* out_h, const int cursor_type);
  /* Set cursor size
     Parameters:
       out_h         - console output handle
       cursor_type   - size of cursor in Linux kernel defines
  */

#if defined __cplusplus
}
#endif

#endif /* __CO_OS_USER_WINNT_OS_CONSOLE_H__ */
