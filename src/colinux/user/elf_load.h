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

#ifndef CO_HOST_API
#include <linux/elf.h>
#else
typedef struct Elf32_Ehdr Elf32_Ehdr;
typedef struct Elf32_Shdr Elf32_Shdr;
typedef struct Elf32_Sym Elf32_Sym;
#endif

typedef	struct co_elf_data {
	/* ELF binary buffer */
	unsigned char *buffer;
	unsigned long size;
		
	/* Elf header and seconds */
	Elf32_Ehdr *header;
	Elf32_Shdr *section_string_table_section;
	Elf32_Shdr *string_table_section;
	Elf32_Shdr *symbol_table_section;
} co_elf_data_t;

struct co_daemon;

co_rc_t co_elf_image_read(co_elf_data_t *pl, void *elf_buf, unsigned long size);
co_rc_t co_elf_image_load(struct co_daemon *daemon);
Elf32_Sym *co_get_symbol_by_name(co_elf_data_t *pl, const char *name);

#endif
