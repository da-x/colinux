/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/manager.h>
#include <colinux/user/cmdline.h>

#define BUFFER_SIZE   (0x100000)

typedef struct co_debug_parameters {
	bool_t download_mode;
	bool_t parse_mode;
} co_debug_parameters_t;

static co_debug_parameters_t parameters;

static void co_debug_download(int fd)
{
	co_manager_handle_t handle;
	co_rc_t rc;

	handle = co_os_manager_open();
	if (handle) {
		char *buffer = (char *)co_os_malloc(BUFFER_SIZE);
		if (buffer) {
			co_manager_ioctl_debug_reader_t debug_reader;
			debug_reader.user_buffer = buffer;
			debug_reader.user_buffer_size = BUFFER_SIZE;
			while (1) {
				debug_reader.filled = 0;
				rc = co_manager_debug_reader(handle, &debug_reader);
				if (!CO_OK(rc)) {
					fprintf(stderr, "log ended: %x\n", rc);
					return;
				}
				write(fd, buffer, debug_reader.filled);
			}
		}
		co_os_free(buffer);
	}

	co_os_manager_close(handle);
}

static void print_xml_text(char *str)
{
	while (*str) {
		char *str_start = str;
		while (*str  &&  *str != '&')
			str++;
		fwrite(str_start, str - str_start, 1, stdout);
		if (!*str)
			break;
		if (*str == '&')
			printf(" ");
		str++;
	}
}

static void co_debug_parse(fd)
{
	printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
	printf("<dump>\n");

	for (;;) {
		co_debug_tlv_t tlv, *ptlv;
		int nread;

		nread = read(fd, &tlv, sizeof(tlv));
		if (nread == 0)
			break;

		if (nread != sizeof(tlv))
			break;

		char block[tlv.length];
		nread = read(fd, &block, tlv.length);
		if (nread != tlv.length)
			break;

		printf("  <log>\n");
		ptlv = (co_debug_tlv_t *)block;
		while ((char *)ptlv < (char *)&block[tlv.length]) {
			switch (ptlv->type) {
			case CO_DEBUG_TYPE_TIMESTAMP: {
				co_debug_timestamp_t *ts = (typeof(ts))(ptlv->value);
				printf("    <timestamp>%08d.%-10d</timestamp>\n", ts->high, ts->low);
				break;
			}
			case CO_DEBUG_TYPE_MODULE: {
				printf("    <module>%s</module>\n", ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_FILE: {
				printf("    <file>%s</file>\n", ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_FUNC: {
				printf("    <function>%s</function>\n", ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_FACILITY: {
				printf("    <facility>%d</facility>\n", *(char *)ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_LINE: {
				printf("    <line>%d</line>\n", *(unsigned long *)ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_LEVEL: {
				printf("    <level>%d</level>\n", *(char *)ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_LOCAL_INDEX: {
				printf("    <local_index>%d</local_index>\n", *(unsigned long *)ptlv->value);
				break;
			}
			case CO_DEBUG_TYPE_STRING: {
				printf("    <string>");
				print_xml_text(ptlv->value);
				printf("</string>\n");
				break;
			}
			}
			
			ptlv = (co_debug_tlv_t *)&ptlv->value[ptlv->length];
		}
		printf("  </log>\n");
	}

	printf("</dump>\n");
}

static co_rc_t co_debug_parse_args(co_command_line_params_t cmdline, co_debug_parameters_t *parameters)
{
	co_rc_t rc;

	parameters->download_mode = PFALSE;

	rc = co_cmdline_params_argumentless_parameter(
		cmdline,
		"-d",
		&parameters->download_mode);

	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_argumentless_parameter(
		cmdline,
		"-p",
		&parameters->parse_mode);

	if (!CO_OK(rc)) 
		return rc;

	return CO_RC(OK);
}

int co_debug_main(int argc, char *argv[])
{
	co_command_line_params_t cmdline;
	co_rc_t rc;

	rc = co_cmdline_params_alloc(argv, argc, &cmdline);
	if (!CO_OK(rc)) {
		co_terminal_print("error parsing args\n");
		return CO_RC(ERROR);
	}

	rc = co_debug_parse_args(cmdline, &parameters);
	if (!CO_OK(rc)) {
		co_terminal_print("error parsing args\n");
		return CO_RC(ERROR);
	}

	if (parameters.download_mode) {
		co_debug_download(1);
	} else if (parameters.parse_mode) {
		co_debug_parse(0);
	}

	return 0;
}
