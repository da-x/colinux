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
	bool_t output_filename_specified;
	char output_filename[0x100];
} co_debug_parameters_t;

static co_debug_parameters_t parameters;

static FILE *output_file = NULL;

static void co_debug_download(void)
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
				fwrite(buffer, 1, debug_reader.filled, output_file);
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

static void parse_tlv(const co_debug_tlv_t *tlv, const char *block)
{
	co_debug_tlv_t *ptlv;
	
	fprintf(output_file, "  <log>\n");
	ptlv = (co_debug_tlv_t *)block;
	while ((char *)ptlv < (char *)&block[tlv->length]) {
		switch (ptlv->type) {
		case CO_DEBUG_TYPE_TIMESTAMP: {
			co_debug_timestamp_t *ts = (typeof(ts))(ptlv->value);
			fprintf(output_file, "    <timestamp>%08u.%-10u</timestamp>\n", 
				(unsigned int)ts->high, (unsigned int)ts->low);
			break;
		}
		case CO_DEBUG_TYPE_MODULE: {
			fprintf(output_file, "    <module>%s</module>\n", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FILE: {
			fprintf(output_file, "    <file>%s</file>\n", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FUNC: {
			fprintf(output_file, "    <function>%s</function>\n", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FACILITY: {
			fprintf(output_file, "    <facility>%d</facility>\n", *(char *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LINE: {
			fprintf(output_file, "    <line>%d</line>\n", *(int *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LEVEL: {
			fprintf(output_file, "    <level>%d</level>\n", *(char *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LOCAL_INDEX: {
			fprintf(output_file, "    <local_index>%d</local_index>\n", *(int *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_STRING: {
			fprintf(output_file, "    <string>");
			print_xml_text(ptlv->value);
			fprintf(output_file, "</string>\n");
			break;
		}
		}
			
		ptlv = (co_debug_tlv_t *)&ptlv->value[ptlv->length];
	}
	fprintf(output_file, "  </log>\n");
	
}

static void parse_tlv_buffer(const char *block, long size)
{
	co_debug_tlv_t *tlv;

	while (size > 0) {
		tlv = (co_debug_tlv_t *)block;
		if (size < sizeof(*tlv))
		    return;

		parse_tlv(tlv, tlv->value); 

		block += sizeof(*tlv) + tlv->length;
		size -= sizeof(*tlv) + tlv->length;
	}
}

static void xml_start(void)
{
	fprintf(output_file, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
	fprintf(output_file, "<dump>\n");
}

static void xml_end(void)
{
	fprintf(output_file, "</dump>\n");
}

static void co_debug_parse(int fd)
{
	xml_start();
	for (;;) {
		co_debug_tlv_t tlv;
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

		parse_tlv(&tlv, block);
	}
	xml_end();
}

void co_debug_download_and_parse(void)
{
	co_manager_handle_t handle;
	co_rc_t rc;

	xml_start();

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

				parse_tlv_buffer(buffer, debug_reader.filled);
			}
		}
		co_os_free(buffer);
	}

	co_os_manager_close(handle);

	xml_end();
}

static co_rc_t co_debug_parse_args(co_command_line_params_t cmdline, co_debug_parameters_t *parameters)
{
	co_rc_t rc;

	parameters->download_mode = PFALSE;
	parameters->parse_mode = PFALSE;
	parameters->output_filename_specified = PFALSE;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-d", &parameters->download_mode);
	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_argumentless_parameter(cmdline, "-p", &parameters->parse_mode);
	if (!CO_OK(rc)) 
		return rc;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-f", &parameters->output_filename_specified,
						      parameters->output_filename, sizeof(parameters->output_filename));
	if (!CO_OK(rc)) 
		return rc;

	return CO_RC(OK);
}

int co_debug_main(int argc, char *argv[])
{
	co_command_line_params_t cmdline;
	co_rc_t rc;

	output_file = stdout;

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

	if (parameters.output_filename_specified) {
		output_file = fopen(parameters.output_filename, "a");
		if (!output_file)
			return CO_RC(ERROR);
	}

	if (parameters.download_mode  &&  parameters.parse_mode) {
		co_debug_download_and_parse();
	} else if (parameters.download_mode) {
		co_debug_download();
	} else if (parameters.parse_mode) {
		co_debug_parse(0);
	}

	if (output_file != stdout) 
		fclose(output_file);

	return 0;
}
