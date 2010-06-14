/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2005 (c)
 *
 * The code in this module was embraced from Linux 2.6.10 (fs/nls):
 *
 *    Native language support--charsets and unicode translations.
 *    By Gordon Chaffee 1996, 1997
 *
 *    Unicode based case conversion 1999 by Wolfram Pienkoss
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

/* OS independent UTF-8 library */

#include "unicode.h"

#include <colinux/common/libc.h>
#include <colinux/os/alloc.h>

/*
 * Sample implementation from Unicode home page.
 * http://www.stonehand.com/unicode/standard/fss-utf.html
 */
struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static struct utf8_table utf8_table[] =
{
	{0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
	{0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
	{0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
	{0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
	{0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
	{0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
	{0,						   /* end of table    */},
};

static int utf8_mbtowc(co_wchar_t *p, const unsigned char *s, int n)
{
	long l;
	int c0, c, nc;
	struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval)
				return -1;
			*p = l;
			return nc;
		}
		if (n <= nc)
			return -1;
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -1;
		l = (l << 6) | c;
	}
	return -1;
}

static int utf8_wctomb(unsigned char *s, co_wchar_t wc, int maxlen)
{
	long l;
	int c, nc;
	struct utf8_table *t;

	if (s == 0)
		return 0;

	l = wc;
	nc = 0;
	for (t = utf8_table; t->cmask && maxlen; t++, maxlen--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = t->cval | (l >> c);
			while (c > 0) {
				c -= 6;
				s++;
				*s = 0x80 | ((l >> c) & 0x3F);
			}
			return nc;
		}
	}
	return -1;
}

int co_utf8_mbstrlen(const char *src)
{
	const unsigned char *ip;
	co_wchar_t op;
	int size, length = 0;
	int maxlen = co_strlen(src);

	ip = (const unsigned char *)src;

	while (maxlen > 0  &&  *ip) {
		length++;

		if (*ip & 0x80) {
			size = utf8_mbtowc(&op, ip, maxlen);
			if (size == -1) {
				ip++;
				maxlen--;
			} else {
				ip += size;
				maxlen -= size;
			}
		} else {
			ip++;
			maxlen--;
		}
	}

	return length;
}

int co_utf8_mcstrlen(co_wchar_t *src)
{
	int length = 0;

	while (*src) {
		length += 1;
		src++;
	}

	return length;
}

co_rc_t co_utf8_mbstowcs(co_wchar_t *dest, const char *src, int maxlen)
{
	co_wchar_t *op;
	const unsigned char *ip;
	int size;

	op = dest;
	ip = (const unsigned char *)src;

	while (maxlen > 0  &&  *ip) {
		if (*ip & 0x80) {
			size = utf8_mbtowc(op, ip, maxlen);
			if (size == -1) {
				*op = '?';
				ip++;
			} else {
				ip += size;
			}
		} else {
			*op = *ip++;
		}
		op++;
		maxlen--;
	}

	*op = 0;

	if (maxlen < 0) {
		return CO_RC(ERROR);
	}

	/* (op - pwcs) */
	return CO_RC(OK);
}

co_rc_t co_utf8_wcstombs(char *dest, const co_wchar_t *src, int maxlen)
{
	const co_wchar_t *ip;
	unsigned char *op;
	int size;

	op = (unsigned char *)dest;
	ip = src;
	maxlen -= 1;

	while (maxlen > 0  &&  *ip) {
		if (*ip > 0x7f) {
			size = utf8_wctomb(op, *ip, maxlen);
			if (size == -1) {
				*op++ = '?';
				maxlen--;
			} else {
				op += size;
				maxlen -= size;
			}
		} else {
			*op++ = (unsigned char)(*ip);
			maxlen--;
		}

		ip++;
	}

	*op = 0;

	if (maxlen < 0) {
		return CO_RC(ERROR);
	}

	/* (op - s) */
	return CO_RC(OK);
}

int co_utf8_wctowbstrlen(const co_wchar_t *src, int maxlen)
{
	const co_wchar_t *ip;
	unsigned char op[8];
	int length = 0;
	int size;

	ip = src;

	while (maxlen > 0  &&  *ip) {
		if (*ip > 0x7f) {
			size = utf8_wctomb(op, *ip, sizeof(op));
			if (size == -1) {
				length += 1;
			} else {
				length += size;
			}
		} else {
			length += 1;
		}

		ip++;
		maxlen--;
	}

	return length;
}

co_rc_t co_utf8_dup_to_wc(const char *src, co_wchar_t **wstring, unsigned long *size_out)
{
	int size;
	co_wchar_t *buffer;
	co_rc_t rc;

	size = co_utf8_mbstrlen(src) + 1;
	buffer = co_os_malloc((size+1)*sizeof(co_wchar_t));
	if (!buffer)
		return CO_RC(OUT_OF_MEMORY);

	rc = co_utf8_mbstowcs(buffer, src, size);
	if (!CO_OK(rc)) {
		co_os_free(buffer);
		return rc;
	}

	if (size_out)
		*size_out = size - 1;

	*wstring = buffer;

	return CO_RC(OK);
}

void co_utf8_free_wc(co_wchar_t *wstring)
{
	co_os_free(wstring);
}
