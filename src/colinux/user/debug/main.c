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
#include <ctype.h>

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
	bool_t settings_change_specified;
	char settings_change[0x100];
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
				co_rc_t rc = co_manager_debug_reader(handle, &debug_reader);
				if (!CO_OK(rc)) {
					fprintf(stderr, "log ended: %x\n", rc);
					return;
				}

				parse_tlv_buffer(buffer, debug_reader.filled);
			}
		}
		co_os_free(buffer);
		co_os_manager_close(handle);
	}

	xml_end();
}

typedef struct {
	char *facility_name;
	unsigned long facility_offset;
} facility_descriptor_t;

static facility_descriptor_t facility_descriptors[] = {
#define X(facility, static_level, default_dynamic_level) \
	{#facility, (((unsigned long)&(((co_debug_levels_t *)0)->facility##_level)))/sizeof(int)},
		CO_DEBUG_LIST
#undef X
		{NULL, 0},
	};

void co_update_settings(void) 
{
	co_manager_handle_t handle;
	handle = co_os_manager_open();
	if (!handle)
		return;

	co_manager_ioctl_debug_levels_t levels = {{0}, };
	levels.modify = PFALSE;

	co_rc_t rc;
	rc = co_manager_debug_levels(handle, &levels);
	if (!CO_OK(rc)) 
		goto out;

	facility_descriptor_t *desc_ptr = facility_descriptors;
	while (desc_ptr->facility_name) {
		int *current_setting = &(((int *)&levels.levels)[desc_ptr->facility_offset]);

		fprintf(stderr, "current facility %s level: %d\n",
			desc_ptr->facility_name, *current_setting);

		if (parameters.settings_change_specified) {
			char *settings_found = NULL;

			char string_to_search[strlen(desc_ptr->facility_name)+2];
			snprintf(string_to_search, sizeof(string_to_search), "%s=", desc_ptr->facility_name);

			settings_found = strstr(parameters.settings_change, string_to_search);
			if (settings_found) {
				char *number_scan  = settings_found + strlen(string_to_search);
				char *number = number_scan;

				while (isdigit(*number_scan))
					number_scan++;
				char number_buf[number_scan - number + 1];
				memcpy(number_buf, number, number_scan - number);
				number_buf[number_scan - number] = '\0';
				int setting = atoi(number);
				
				*current_setting = setting;
				
				fprintf(stderr, "setting facility %s to %d\n", 
					desc_ptr->facility_name, setting);
			}
		}
		
		desc_ptr++;
	}


	levels.modify = PTRUE;
	rc = co_manager_debug_levels(handle, &levels);

out:
	co_os_manager_close(handle);
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

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-s", &parameters->settings_change_specified,
						      parameters->settings_change, sizeof(parameters->settings_change));
	if (!CO_OK(rc)) 
		return rc;


	return CO_RC(OK);
}

static void syntax(void)
{
	printf("colinux-debug-daemon\n");
	printf("syntax: \n");
	printf("\n");
	printf("    colinux-daemon [-h] [-c config.xml] [-d]\n");
	printf("\n");
	printf("      -d             Download debug information on the fly\n");
	printf("      -p             Parse the debug information and output an XML\n");
	printf("                     Without -d, uses standard input, otherwise parses\n");
	printf("                     the downloaded information\n");
	printf("      -f [filename]  File to append the output instead of writing to\n");
	printf("                     standard output\n");
	printf("      -s level=num,level2=num2,...\n");
	printf("                     Change the levels of the given debug facilities\n");
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

	co_update_settings();

	if (parameters.download_mode  &&  parameters.parse_mode) {
		co_debug_download_and_parse();
	} else if (parameters.download_mode) {
		co_debug_download();
	} else if (parameters.parse_mode) {
		co_debug_parse(0);
	} else {
		syntax();
	}

	if (output_file != stdout) 
		fclose(output_file);

	return 0;
}
