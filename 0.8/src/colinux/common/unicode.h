#ifndef __COLINUX_COMMON_UNICODE_H__
#define __COLINUX_COMMON_UNICODE_H__

#include <colinux/common/common.h>

typedef unsigned short co_wchar_t;

extern co_rc_t co_utf8_mbstowcs(co_wchar_t *dest, const char *src, int maxlen);
extern co_rc_t co_utf8_wcstombs(char *dest, const co_wchar_t *src, int maxlen);

extern co_rc_t co_utf8_dup_to_wc(const char *src, co_wchar_t **wstring, unsigned long *size_out);
extern void co_utf8_free_wc(co_wchar_t *wstring);

extern int co_utf8_wctowbstrlen(const co_wchar_t *src, int maxlen);
extern int co_utf8_mbstrlen(const char *src);
extern int co_utf8_mcstrlen(co_wchar_t *src);

#endif
