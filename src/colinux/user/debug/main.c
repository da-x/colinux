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
#include <signal.h>

#include <colinux/os/alloc.h>
#include <colinux/os/timer.h>
#include <colinux/os/user/misc.h>
#include <colinux/user/manager.h>
#include <colinux/user/cmdline.h>
#include <colinux/common/libc.h>

#define BUFFER_SIZE   (0x100000)

typedef struct co_debug_parameters {
	bool_t download_mode;
	bool_t parse_mode;
	bool_t output_filename_specified;
	char output_filename[0x100];
	bool_t settings_change_specified;
	char settings_change[0x100];
	bool_t network_server_specified;
	char network_server[0x100];
	bool_t rc_specified;
	char rc_str[20];
} co_debug_parameters_t;

static co_debug_parameters_t parameters;

static FILE *output_file;

static void sig_handle(int signo);	/* Forward declaration */

static void co_debug_download(void)
{
	co_manager_handle_t handle;
	co_rc_t rc;

	handle = co_os_manager_open();
	if (handle) {
		char *buffer = co_os_malloc(BUFFER_SIZE);
		if (buffer) {
			co_manager_ioctl_debug_reader_t debug_reader;
			debug_reader.user_buffer = buffer;
			debug_reader.user_buffer_size = BUFFER_SIZE;
			while (1) {
				debug_reader.filled = 0;
				rc = co_manager_debug_reader(handle, &debug_reader);
				if (!CO_OK(rc)) {
					fprintf(stderr, "log ended: %x\n", (int)rc);
					break;
				}

				fwrite(buffer, 1, debug_reader.filled, output_file);
			}
			co_os_free(buffer);
		}
		co_os_manager_close(handle);
	}

}

static void co_debug_download_to_network(void)
{
	co_manager_handle_t handle;
	co_rc_t rc;
	int sock;
	int port = 63000;

	fprintf(stderr, "sending UDP packets to %s:%d\n", parameters.network_server, port);

	sock = co_udp_socket_connect(parameters.network_server, port);
	if (sock == -1)
		return;

	handle = co_os_manager_open();
	if (!handle)
		goto error_out1;

	char *buffer = co_os_malloc(BUFFER_SIZE);
	if (!buffer)
		goto error_out2;

	co_manager_ioctl_debug_reader_t debug_reader;
	debug_reader.user_buffer = buffer;
	debug_reader.user_buffer_size = BUFFER_SIZE;
	while (1) {
		debug_reader.filled = 0;
		rc = co_manager_debug_reader(handle, &debug_reader);
		if (!CO_OK(rc)) {
			fprintf(stderr, "log ended: %x\n", (int)rc);
			break;
		}

		co_debug_tlv_t *tlv;
		unsigned long size = debug_reader.filled;
		char *block = debug_reader.user_buffer;

		while (size > 0) {
			tlv = (co_debug_tlv_t *)block;
			if (size < sizeof(*tlv))
				goto error_out3;

			co_udp_socket_send(sock, (char *)tlv, tlv->length + sizeof(*tlv));

			block += sizeof(*tlv) + tlv->length;
			size -= sizeof(*tlv) + tlv->length;
		}
	}
error_out3:
	co_os_free(buffer);
error_out2:
	co_os_manager_close(handle);
error_out1:
	co_udp_socket_close(sock);
}

static void print_xml_text(const char *str)
{
	while (*str) {
		switch (*str) {
			case '&': fwrite("&amp;", 5, 1, output_file); break;
			case '<': fwrite("&lt;", 4, 1, output_file); break;
			case '>': fwrite("&gt;", 4, 1, output_file); break;
			default: putc(*str, output_file);
		}
		str++;
	}
}

static void parse_tlv(const co_debug_tlv_t *tlv, const char *block)
{
	const co_debug_tlv_t *ptlv;

	fprintf(output_file, "  <log ");
	ptlv = (const co_debug_tlv_t *)block;
	while ((char *)ptlv < (char *)&block[tlv->length]) {
		switch (ptlv->type) {
		case CO_DEBUG_TYPE_TIMESTAMP: {
			co_timestamp_t *ts = (typeof(ts))(ptlv->value);
			fprintf(output_file, " timestamp=\"%08u.%-10u\"",
				(unsigned int)ts->high, (unsigned int)ts->low);
			break;
		}
		case CO_DEBUG_TYPE_MODULE: {
			fprintf(output_file, " module=\"%s\"", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FILE: {
			fprintf(output_file, " file=\"%s\"", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FUNC: {
			fprintf(output_file, " function=\"%s\"", ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_FACILITY: {
			fprintf(output_file, " facility=\"%d\"", *(char *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LINE: {
			fprintf(output_file, " line=\"%d\"", *(int *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LEVEL: {
			fprintf(output_file, " level=\"%d\"", *(char *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_LOCAL_INDEX: {
			fprintf(output_file, " local_index=\"%d\"", *(int *)ptlv->value);
			break;
		}
		case CO_DEBUG_TYPE_DRIVER_INDEX: {
			fprintf(output_file, " driver_index=\"%d\"", *(int *)ptlv->value);
			break;
		}
		}
		ptlv = (co_debug_tlv_t *)&ptlv->value[ptlv->length];
	}
	fprintf(output_file, ">\n");

	ptlv = (const co_debug_tlv_t *)block;
	while ((char *)ptlv < (char *)&block[tlv->length]) {
		switch (ptlv->type) {
		case CO_DEBUG_TYPE_STRING: {
			fprintf(output_file, "<string>");
			print_xml_text(ptlv->value);
			fprintf(output_file, "</string>\n");
			break;
		}
		}
		ptlv = (co_debug_tlv_t *)&ptlv->value[ptlv->length];
	}

	fprintf(output_file, "</log>\n");

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

	signal (SIGINT, sig_handle);	/* CTRL-C on command line */
	signal (SIGTERM, sig_handle);	/* Termination signal (Linux) */
}

static void xml_end(void)
{
	fprintf(output_file, "</dump>\n");
}

static void sig_handle(int signo)
{
	xml_end();
	fprintf (stderr, "\ncolinux-debug-daemon terminated (%d)\n", signo);
	exit (signo);
}

static int read_helper (int fd, char * bf, int len)
{
	int total_rd = 0;
	int rd;

	/* Input file can pipe, or unflushed device */
	/* An incomplete read is not the end of debug log */
	while ((rd = read (fd, bf, len)) > 0 && len > 0) {
	    total_rd += rd;
	    bf += rd;
	    len -= rd;
	}

	return (total_rd);
}

static void co_debug_parse(int fd)
{
	xml_start();
	for (;;) {
		co_debug_tlv_t tlv;
		int nread;

		nread = read(fd, &tlv, sizeof(tlv));
		if (nread != sizeof(tlv) || tlv.length == 0)
			break;

		char block[tlv.length];
		nread = read_helper(fd, (void*)&block, tlv.length);
		if (nread != tlv.length)
			break;

		parse_tlv(&tlv, block);

		/* Flush every block, if stdout not redirected */
		if (output_file != stdout)
			fflush (output_file);
	}
	xml_end();
}

void co_debug_download_and_parse(void)
{
	co_manager_handle_t handle;
	xml_start();

	handle = co_os_manager_open();
	if (handle) {
		char *buffer = co_os_malloc(BUFFER_SIZE);
		if (buffer) {
			co_manager_ioctl_debug_reader_t debug_reader;
			debug_reader.user_buffer = buffer;
			debug_reader.user_buffer_size = BUFFER_SIZE;
			while (1) {
				debug_reader.filled = 0;
				co_rc_t rc = co_manager_debug_reader(handle, &debug_reader);
				if (!CO_OK(rc)) {
					fprintf(stderr, "log ended: %x\n", (int)rc);
					return;
				}

				parse_tlv_buffer(buffer, debug_reader.filled);

				/* Flush, if stdout not redirected */
				if (output_file != stdout)
					fflush (output_file);
			}
			co_os_free(buffer);
		}
		co_os_manager_close(handle);
	}

	xml_end();
}

typedef struct {
	char *facility_name;
	unsigned long facility_offset;
} facility_descriptor_t;

#ifdef COLINUX_DEBUG
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
			const char *settings_found = NULL;

			char string_to_search[co_strlen(desc_ptr->facility_name)+2];
			snprintf(string_to_search, sizeof(string_to_search), "%s=", desc_ptr->facility_name);

			settings_found = co_strstr(parameters.settings_change, string_to_search);
			if (settings_found) {
				const char *number_scan = settings_found + co_strlen(string_to_search);
				const char *number = number_scan;

				while (isdigit(*number_scan))
					number_scan++;
				char number_buf[number_scan - number + 1];
				co_memcpy(number_buf, number, number_scan - number);
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
#else
void co_update_settings(void) { return; }
#endif


static co_rc_t co_debug_parse_args(co_command_line_params_t cmdline, co_debug_parameters_t *parameters)
{
	co_rc_t rc;

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

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-n", &parameters->network_server_specified,
						      parameters->network_server, sizeof(parameters->network_server));
	if (!CO_OK(rc))
		return rc;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, "-e", &parameters->rc_specified,
						      parameters->rc_str, sizeof(parameters->rc_str));
	if (!CO_OK(rc))
		return rc;

	return CO_RC(OK);
}

static void syntax(void)
{
	printf("colinux-debug-daemon\n");
	printf("syntax: \n");
	printf("\n");
	printf("    colinux-debug-daemon [-d] [-f filename | -n ipaddress] [-p] [-s levels] | -e exitcode | -h\n");
	printf("\n");
	printf("      -d              Download debug information on the fly from driver.\n");
	printf("                      Without -d, uses standard input.\n");
	printf("      -p              Parse the debug information and output an XML\n");
	printf("      -f filename     File to append the output instead of writing to\n");
	printf("                      standard output. Write with flush every line.\n");
	printf("      -s level=num,level2=num2,...\n");
	printf("                      Change the levels of the given debug facilities\n");
	printf("      -n ipaddress    Send logs as UDP packets to ipaddress:63000\n");
	printf("                      (requires -d)\n");
	printf("      -e exitcode     Translate exitcode into human readable format.\n");
	printf("      -h              This help text\n");
	printf("\n");
}

co_rc_t co_debug_main(int argc, char *argv[])
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

	if (parameters.rc_specified) {
		char buf[0x100];
		unsigned long exitcode;

		exitcode = strtoul(parameters.rc_str, NULL, 16);
		co_rc_format_error((co_rc_t)exitcode, buf, sizeof(buf));

		printf(" Translate error code %lx\n", exitcode);
		printf(" %s\n", buf);
		return CO_RC(OK);
	}

#ifndef COLINUX_DEBUG
	fprintf(stderr, "Warning: Some informations are not available, COLINUX_DEBUG was not compiled in.\n");
#endif

	if (parameters.output_filename_specified) {
		output_file = fopen(parameters.output_filename, "ab");
		if (!output_file)
			return CO_RC(ERROR);
	}

	if (parameters.settings_change_specified)
		co_update_settings();

	if (parameters.download_mode  &&  parameters.network_server_specified) {
		co_debug_download_to_network();
	} else if (parameters.download_mode  &&  parameters.parse_mode) {
		co_debug_download_and_parse();
	} else if (parameters.download_mode) {
		co_debug_download();
	} else if (parameters.parse_mode) {
		co_debug_parse(STDIN_FILENO);
	} else {
		syntax();
	}

	if (output_file != stdout)
		fclose(output_file);

	return CO_RC(OK);
}
