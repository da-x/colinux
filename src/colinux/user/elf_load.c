/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include "daemon.h"

#include <string.h>

CO_TRACE_STOP;

/*
 * This code in this file basically allows to enumerate loadable
 * sections of the given ELF image file. The sections that interest
 * us most in the vmlinux are the sections that are actually 
 * loadable (or allocatable, like the bss).
 *
 * Previously, I used to allocate each section in the kernel 
 * separately when it was loaded by the ioctl below. But now, I just
 * figure out the entire size of the loaded kernel by looking at 
 * the addresses of the start and end symbols, allocating the whole
 * kernel in a big chunk, and then use memset and memcpy in the 
 * per section ioctl handler in order to initialize the loaded image.
 *
 * Optionally, it is possible to do this in a completely different 
 * way: load the entire vmlinux file to kernel memory and then do 
 * all the processing there (i.e, map the physical pages of the 
 * loaded vmlinux memory according the section headers, initialize
 * the bss, get rid of unused sections, etc.).
 */ 
Elf32_Phdr *co_get_program_header(co_elf_data_t *pl, long index)
{
	return (Elf32_Phdr *)(pl->buffer + pl->header->e_phoff +  
			      (pl->header->e_phentsize * index));
}

unsigned long co_get_program_count(co_elf_data_t *pl)
{
	return pl->header->e_phnum;
}

Elf32_Shdr *co_get_section_header(co_elf_data_t *pl, long index)
{
	return (Elf32_Shdr *)(pl->buffer + pl->header->e_shoff + 
			      (pl->header->e_shentsize * (index)));
}

unsigned long co_get_section_count(co_elf_data_t *pl)
{
	return pl->header->e_shnum;
}

char *co_get_at_offset(co_elf_data_t *pl, Elf32_Shdr *section, unsigned long index)
{
	return &pl->buffer[section->sh_offset + index];
}

char *co_get_section_name(co_elf_data_t *pl, Elf32_Shdr *section)
{
	return co_get_at_offset(pl, pl->section_string_table_section, section->sh_name);
}

Elf32_Shdr *co_get_section_by_name(co_elf_data_t *pl, const char *name)
{
	unsigned long index;
	Elf32_Shdr *section;

	for (index=1; index < co_get_section_count(pl); index++) { 
		section = co_get_section_header(pl, index);

		if (strcmp(co_get_section_name(pl, section), name) == 0)
			return section;
	}    
	return NULL;
}

Elf32_Sym *co_get_symbol(co_elf_data_t *pl, unsigned long index)
{
	return (Elf32_Sym *)
		co_get_at_offset(pl, 
				    pl->symbol_table_section, index*sizeof(Elf32_Sym));
}

char *co_get_string(co_elf_data_t *pl, unsigned long index)
{
	return co_get_at_offset(pl, pl->string_table_section, index);
}

Elf32_Sym *co_get_symbol_by_name(co_elf_data_t *pl, const char *name)
{
	long index =0 ; 
	unsigned long symbols;

	symbols = pl->symbol_table_section->sh_size / sizeof(Elf32_Sym);
    
	for (index=0; index < symbols; index++) { 
		Elf32_Sym *symbol = co_get_symbol(pl, index);
	    
		if (strcmp(name, co_get_string(pl, symbol->st_name)) == 0)
			return symbol;
	}

	return NULL;
}

unsigned long co_get_symbol_offset(co_elf_data_t *pl, Elf32_Sym *symbol)
{
	/* It doesn't work with absolute symbols, need to fix that? */

	Elf32_Shdr *section;

	section = co_get_section_header(pl, symbol->st_shndx);

	return symbol->st_value - section->sh_addr;
}

co_rc_t co_elf_image_read(co_elf_data_t *pl, void *elf_buf, unsigned long size)
{
	pl->header = (Elf32_Ehdr *)elf_buf;
	pl->buffer = elf_buf;
	pl->size = size;
    
	pl->section_string_table_section =\
		co_get_section_header(pl, pl->header->e_shstrndx);
    
	if (pl->section_string_table_section == NULL)
		return CO_RC(ERROR);

	pl->string_table_section = co_get_section_by_name(pl, ".strtab");
	if (pl->string_table_section == NULL)
		return CO_RC(ERROR);

	pl->symbol_table_section = co_get_section_by_name(pl, ".symtab");
	if (pl->symbol_table_section == NULL)
		return CO_RC(ERROR);
    
	return CO_RC(OK);
}

co_rc_t co_section_load(co_daemon_t *daemon, unsigned long index)
{
	Elf32_Shdr *section;
	co_monitor_ioctl_load_section_t params;
	co_rc_t rc;

	section = co_get_section_header(&daemon->elf_data, index);
	if (section->sh_flags & SHF_ALLOC) {
		if (section->sh_type == SHT_NOBITS)
			params.user_ptr = NULL;
		else
			params.user_ptr = co_get_at_offset(&daemon->elf_data, section, 0);
	
		params.address = section->sh_addr;
		params.size = section->sh_size;
		params.index = index;
	
		/*
		 * Load each ELF section to kernel space separately.
		 */
		rc = co_user_monitor_load_section(daemon->monitor, &params);
		if (!CO_OK(rc))
			return rc;
	}

	return CO_RC(OK);
}

/*
 * Load all ELF sections to host OS kernel memory.
 */
co_rc_t co_elf_image_load(co_daemon_t *daemon)
{
	unsigned long index;
	unsigned long sections;
	co_rc_t rc;

	sections = co_get_section_count(&daemon->elf_data);

	for (index=1; index < sections; index++) {
		rc = co_section_load(daemon, index);
		
		if (!CO_OK(rc))
			return rc;
	}

	return CO_RC(OK);
}

CO_TRACE_CONTINUE;
