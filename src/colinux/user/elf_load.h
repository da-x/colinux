/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __COLINUX_USER_ELF_LOAD_H__
#define __COLINUX_USER_ELF_LOAD_H__

#include <colinux/common/common.h>

typedef	struct co_elf_data co_elf_data_t;
typedef	struct co_elf_symbol co_elf_symbol_t;

struct co_daemon;

extern co_rc_t co_elf_image_read(co_elf_data_t **pl, void *elf_buf, unsigned long size);
extern co_rc_t co_elf_image_load(struct co_daemon *daemon);
extern co_elf_symbol_t *co_get_symbol_by_name(co_elf_data_t *pl, const char *name);
extern void *co_elf_get_symbol_data(co_elf_data_t *pl, co_elf_symbol_t *symbol);
extern unsigned long co_elf_get_symbol_value(co_elf_symbol_t *symbol);

#endif
