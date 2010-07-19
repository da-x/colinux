/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "cmdline.h"

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/file.h>

struct co_command_line_params {
	int argc;
	char **argv;
};

typedef enum {
	PARAM_PARSE_STATE_INIT=0,
	PARAM_PARSE_STATE_WHITESPACE,
	PARAM_PARSE_STATE_COMMENT,
	PARAM_PARSE_STATE_PARAM,
	PARAM_PARSE_STATE_PARAM_EQUAL,
	PARAM_PARSE_STATE_PARAM_EQUAL_STR,
	PARAM_PARSE_STATE_NUM_STATES,
} param_parse_state_t;

typedef enum {
	PARAM_PARSE_MARK_NONE=0,
	PARAM_PARSE_MARK_START_OF_PARAM,
	PARAM_PARSE_MARK_END_OF_PARAM,
} param_parse_mark_t;

typedef struct {
	param_parse_state_t new_state;
	param_parse_mark_t mark;
} param_parse_state_action_t;

static param_parse_state_action_t parser_states[PARAM_PARSE_STATE_NUM_STATES][0x100] = {
	[PARAM_PARSE_STATE_INIT] = {
		[0x00 ... 0xff] = {PARAM_PARSE_STATE_INIT, },
		[' '] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\t'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\n'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\r'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['a' ... 'z'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['A' ... 'Z'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['0' ... '9'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['.'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['#'] = {PARAM_PARSE_STATE_COMMENT, },
	},
	[PARAM_PARSE_STATE_COMMENT] = {
		[0x00 ... 0xff] = {PARAM_PARSE_STATE_COMMENT, },
		['\n'] = {PARAM_PARSE_STATE_INIT, },
	},
	[PARAM_PARSE_STATE_WHITESPACE] = {
		[0x00 ... 0xff] = {PARAM_PARSE_STATE_INIT, },
		[' '] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\t'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\n'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['\r'] = {PARAM_PARSE_STATE_WHITESPACE, },
		['a' ... 'z'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['A' ... 'Z'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['0' ... '9'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['.'] = {PARAM_PARSE_STATE_PARAM, PARAM_PARSE_MARK_START_OF_PARAM},
		['#'] = {PARAM_PARSE_STATE_COMMENT, },
	},
	[PARAM_PARSE_STATE_PARAM] = {
		[0x00 ... 0x20] = {PARAM_PARSE_STATE_INIT, PARAM_PARSE_MARK_END_OF_PARAM},
		[0x21 ... 0xff] = {PARAM_PARSE_STATE_PARAM, },
		['='] = {PARAM_PARSE_STATE_PARAM_EQUAL, },
		['"'] = {PARAM_PARSE_STATE_PARAM_EQUAL_STR, },
		['#'] = {PARAM_PARSE_STATE_COMMENT, PARAM_PARSE_MARK_END_OF_PARAM},
	},
	[PARAM_PARSE_STATE_PARAM_EQUAL] = {
		[0x00 ... 0x20] = {PARAM_PARSE_STATE_INIT, PARAM_PARSE_MARK_END_OF_PARAM},
		[0x21 ... 0xff] = {PARAM_PARSE_STATE_PARAM_EQUAL, },
		['"'] = {PARAM_PARSE_STATE_PARAM_EQUAL_STR, },
		['#'] = {PARAM_PARSE_STATE_COMMENT, PARAM_PARSE_MARK_END_OF_PARAM},
	},
	[PARAM_PARSE_STATE_PARAM_EQUAL_STR] = {
		[0x00 ... 0x1f] = {PARAM_PARSE_STATE_INIT, PARAM_PARSE_MARK_END_OF_PARAM},
		[0x20 ... 0xff] = {PARAM_PARSE_STATE_PARAM_EQUAL_STR, },
		['"'] = {PARAM_PARSE_STATE_PARAM, },
		['#'] = {PARAM_PARSE_STATE_COMMENT, PARAM_PARSE_MARK_END_OF_PARAM},
	},
};

static co_rc_t dup_param(const char *source, int len, char **out_dest)
{
	*out_dest = co_os_malloc(len + 1);
	if (!*out_dest)
		return CO_RC(OUT_OF_MEMORY);

	co_memcpy(*out_dest, source, len);
	(*out_dest)[len] = '\0';
	return CO_RC(OK);
}

static co_rc_t get_params_list(char *filename, int *count, char ***list_output)
{
	co_rc_t rc;
	char *buf;
	unsigned long size;

	rc = co_os_file_load(filename, &buf, &size, 0);
	if (!CO_OK(rc)) {
		co_terminal_print("error loading config file '%s'\n", filename);
		return rc;
	}

	if (size > 0) {
		char *filescan = buf, *param_start = NULL, *param_end, cur_char;
		param_parse_state_t state = PARAM_PARSE_STATE_INIT;
		param_parse_state_action_t *action;

		do {
			if (filescan < &buf[size])
				cur_char = *filescan;
			else
				cur_char = 0;
			action = &parser_states[state][(unsigned char)cur_char];
			state = action->new_state;

			switch (action->mark) {
			case PARAM_PARSE_MARK_START_OF_PARAM:
				param_start = filescan;
				break;
			case PARAM_PARSE_MARK_END_OF_PARAM:
				param_end = filescan;
				if (list_output) {
					if (!param_start)
						return CO_RC(ERROR);

					rc = dup_param(param_start, param_end - param_start, *list_output);
					if (!CO_OK(rc))
						break;
					(*list_output)++;
					param_start = NULL;
				}
				if (count)
					(*count)++;
				break;
			default:
				break;
			}
			filescan++;
		} while (cur_char);
	}

	co_os_file_free(buf);

	return CO_RC(OK);
}

static co_rc_t eval_params_file(co_command_line_params_t cmdline)
{
	int count = 0, i = 0;
	char **argv = NULL;
	char **argv_scan = NULL;
	co_rc_t rc = CO_RC(OK);

	for (i=0; i < cmdline->argc; i++) {
		if (co_strlen(cmdline->argv[i]) >= 1  &&  cmdline->argv[i][0] == '@') {
			rc = get_params_list(&cmdline->argv[i][1], &count, NULL);
			if (!CO_OK(rc))
				return rc;

			continue;
		}
		count++;
	}

	argv = co_os_malloc(count*sizeof(char *));
	if (!argv)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(argv, 0, count*sizeof(char *));
	argv_scan = argv;
	count = 0;
	for (i=0; i < cmdline->argc; i++) {
		if (co_strlen(cmdline->argv[i]) >= 1  &&  cmdline->argv[i][0] == '@') {
			rc = get_params_list(&cmdline->argv[i][1], &count, &argv_scan);
			if (!CO_OK(rc))
				break;
			continue;
		}

		rc = dup_param(cmdline->argv[i], co_strlen(cmdline->argv[i]), argv_scan);
		if (!CO_OK(rc))
			break;

		argv_scan++;
		count++;
	}

	if (!CO_OK(rc)) {
		for (i=0; i < count; i++)
			co_os_free(argv[i]);
		co_os_free(argv);
		return rc;
	}

	co_os_free(cmdline->argv);
	cmdline->argv = argv;
	cmdline->argc = count;

	return CO_RC(OK);
}

co_rc_t co_cmdline_params_alloc(char **argv, int argc, co_command_line_params_t *cmdline_out)
{
	co_command_line_params_t cmdline;
	unsigned long argv_size;
	co_rc_t rc;

	cmdline = (typeof(cmdline))(malloc(sizeof(struct co_command_line_params)));
	if (cmdline == NULL)
		return CO_RC(OUT_OF_MEMORY);

	argv_size = (sizeof(char *))*(argc+1);
	cmdline->argv = co_os_malloc(argv_size);
	if (!cmdline->argv) {
		co_os_free(cmdline);
		return CO_RC(OUT_OF_MEMORY);
	}

	cmdline->argc = argc;
	co_memcpy(cmdline->argv, argv, (sizeof(char*))*argc);

	rc = eval_params_file(cmdline);
	if (!CO_OK(rc)) {
		co_os_free(cmdline->argv);
		co_os_free(cmdline);
		return rc;
	}

	*cmdline_out = cmdline;

	return CO_RC(OK);
}

co_rc_t co_cmdline_params_argumentless_parameter(co_command_line_params_t cmdline,
						 const char *name,
						 bool_t *out_exists)
{
	int i;

	*out_exists = PFALSE;

	for (i=0; i < cmdline->argc; i++) {
		if (!cmdline->argv[i])
			continue;

		if (co_strcmp(cmdline->argv[i], name) == 0) {
			co_os_free(cmdline->argv[i]);
			cmdline->argv[i] = NULL;
			*out_exists = PTRUE;
			return CO_RC(OK);
		}
	}

	return CO_RC(OK);
}

static co_rc_t co_cmdline_get_next_equality_wrapper(co_command_line_params_t cmdline, const char *expected_prefix, int max_suffix_len,
				     char *key, int key_size, char **pp_value, int value_size, bool_t *out_exists)
{
	static char *value;
	int i;
	int length;
	int prefix_len = co_strlen(expected_prefix);

	*out_exists = PFALSE;

	for (i=0; i < cmdline->argc; i++) {
		const char *key_found;
		char *arg = cmdline->argv[i];
		int key_len;

		if (!cmdline->argv[i])
			continue;

		key_found = co_strstr(cmdline->argv[i], "=");
		if (!key_found)
			continue;

		if (co_strncmp(expected_prefix, cmdline->argv[i], prefix_len))
			continue;

		cmdline->argv[i] = NULL;

		key_len = (key_found - arg) - prefix_len;
		if (key_len > 0) {
			if (key_size < key_len + 1) {
				co_terminal_print("cmdline: Index not allowed for '%s'\n",
					  expected_prefix);
				co_os_free(arg);
				return CO_RC(INVALID_PARAMETER);
			}

			co_memcpy(key, prefix_len + arg, key_len);
			key[key_len] = '\0';
		}

		/* string start and size */
		key_found++;
		length = co_strlen(key_found);

		if (!length) {
			co_terminal_print("cmdline: Missing arguments for '%s%s'\n",
					  expected_prefix, key_len ? key : "");
			co_os_free(arg);
			return CO_RC(INVALID_PARAMETER);
		}


		if (!value_size) {
			/* free old value */
			if (value)
				co_os_free(value);
			/* dynamic alloc value buffer */
			value = co_os_malloc(length+1);
			if (!value) {
				co_os_free(arg);
				return CO_RC(OUT_OF_MEMORY);
			}
			*pp_value = value;
		} else if (length >= value_size-1) {
			co_os_free(arg);
			return CO_RC(ERROR);
		}

		co_memcpy(*pp_value, key_found, length);
		(*pp_value)[length] = '\0';
		co_os_free(arg);

		*out_exists = PTRUE;
		break;
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_get_next_equality(co_command_line_params_t cmdline, const char *expected_prefix, int max_suffix_len,
				     char *key, int key_size, char *value, int value_size, bool_t *out_exists)
{
	return co_cmdline_get_next_equality_wrapper(cmdline, expected_prefix, max_suffix_len,
				     key, key_size, &value, value_size, out_exists);
}

co_rc_t co_cmdline_get_next_equality_alloc(co_command_line_params_t cmdline, const char *expected_prefix, int max_suffix_len,
				     char *key, int key_size, char **pp_value, bool_t *out_exists)
{
	return co_cmdline_get_next_equality_wrapper(cmdline, expected_prefix, max_suffix_len,
				     key, key_size, pp_value, 0, out_exists);
}

co_rc_t co_cmdline_get_next_equality_int_prefix(co_command_line_params_t cmdline, const char *expected_prefix,
						unsigned int *key_int, unsigned int max_index,
						char **pp_value, bool_t *out_exists)
{
	char number[11];
	co_rc_t rc;

	rc = co_cmdline_get_next_equality_alloc(cmdline, expected_prefix, sizeof(number) - 1,
					  number, sizeof(number), pp_value, out_exists);
	if (!CO_OK(rc))
		return rc;

	if (*out_exists) {
		char *number_parse = NULL;
		unsigned int value_int;

		value_int = strtoul(number, &number_parse, 10);
		if (number_parse == number) {
			/* not a number */
			co_terminal_print("cmdline: suffix not a number\n");
			return CO_RC(INVALID_PARAMETER);
		}

		if (value_int >= max_index) {
			co_terminal_print("cmdline: invalid %s index: %d\n",
					  expected_prefix, value_int);
			return CO_RC(INVALID_PARAMETER);
		}

		*key_int = value_int;
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_get_next_equality_int_value(co_command_line_params_t cmdline, const char *expected_prefix,
					       unsigned int *value_int, bool_t *out_exists)
{
	char value[20];
	co_rc_t rc;

	rc = co_cmdline_get_next_equality(cmdline, expected_prefix, 0, NULL, 0, value, sizeof(value), out_exists);
	if (!CO_OK(rc))
		return rc;

	if (*out_exists) {
		char *value_parse = NULL;

		*value_int = strtoul(value, &value_parse, 10);
		if (value_parse == value) {
			/* not a number */
			return CO_RC(ERROR);
		}
	}

	return CO_RC(OK);
}



static co_rc_t one_arugment_parameter(co_command_line_params_t cmdline,
				      const char *name,
				      bool_t *argument,
				      bool_t *out_exists,
				      char *out_arg_buf, int arg_size)
{
	int i;

	if (out_exists)
		*out_exists = PFALSE;

	for (i=0; i < cmdline->argc; i++) {
		bool_t argument_specified = PTRUE;

		if (!cmdline->argv[i])
			continue;

		if (co_strcmp(cmdline->argv[i], name) != 0)
			continue;

		if (i >= cmdline->argc -1)
			argument_specified = PFALSE;
		else if (cmdline->argv[i+1] == NULL)
			argument_specified = PFALSE;
		else if (cmdline->argv[i+1][0] == '-')
			argument_specified = PFALSE;

		if (*argument) {
			if (!argument_specified) {
				co_terminal_print("error, no argument passed to command line switch '%s'\n", name);
				return CO_RC(ERROR);
			}
		}

		*argument = argument_specified;
		if (argument_specified) {
			co_snprintf(out_arg_buf, arg_size, "%s", cmdline->argv[i+1]);
			co_os_free(cmdline->argv[i+1]);
			cmdline->argv[i+1] = NULL;
		}

		co_os_free(cmdline->argv[i]);
		cmdline->argv[i] = NULL;
		if (out_exists)
			*out_exists = PTRUE;

		return CO_RC(OK);
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_params_one_arugment_parameter(co_command_line_params_t cmdline,
						 const char *name,
						 bool_t *out_exists,
						 char *out_arg_buf, int arg_size)
{
	bool_t argument = PTRUE;
	return one_arugment_parameter(cmdline, name, &argument, out_exists, out_arg_buf, arg_size);
}

co_rc_t co_cmdline_params_one_arugment_int_parameter(co_command_line_params_t cmdline,
						     const char *name,
						     bool_t *out_exists, unsigned int *out_int)
{
	char arg_buf[0x20];
	co_rc_t rc;
	char *end_ptr;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, name, out_exists, arg_buf, sizeof(arg_buf));
	if (!CO_OK(rc))
		return rc;

	if (out_exists && *out_exists) {
		*out_int = strtoul(arg_buf, &end_ptr, 10);
		if (end_ptr == arg_buf)
			return CO_RC(ERROR);
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_params_one_optional_arugment_parameter(co_command_line_params_t cmdline,
							  const char *name,
							  bool_t *out_exists,
							  char *out_arg_buf, int arg_size)
{
	bool_t argument = PFALSE;
	return one_arugment_parameter(cmdline, name, &argument, out_exists, out_arg_buf, arg_size);
}

co_rc_t co_cmdline_params_check_for_no_unparsed_parameters(co_command_line_params_t cmdline,
							   bool_t print)
{
	int i, count = 0;

	for (i=0; i < cmdline->argc; i++) {
		if (!cmdline->argv[i])
			continue;

		if (print)
			co_terminal_print("unknown command line parameter %d: '%s'\n", i+1, cmdline->argv[i]);
		count++;
	}

	if (count != 0)
		return CO_RC(ERROR);

	return CO_RC(OK);
}

co_rc_t co_cmdline_params_format_remaining_parameters(co_command_line_params_t cmdline,
						      char *str_out, int size)
{
	int i;
	char *str_out_orig = str_out;

	str_out_orig[0] = '\0';

	for (i=0; i < cmdline->argc; i++) {
		int param_size;

		if (!cmdline->argv[i])
			continue;

		co_snprintf(str_out, size, (str_out_orig[0] == '\0') ? "%s" : " %s", cmdline->argv[i]);

		param_size = co_strlen(str_out);

		str_out += param_size;
		size -= param_size;

		if (size == 0)
			break;

		co_os_free(cmdline->argv[i]);
		cmdline->argv[i] = NULL;
	}

	return CO_RC(OK);
}

void co_cmdline_params_free(co_command_line_params_t cmdline)
{
	int i;
	for (i=0; i < cmdline->argc; i++) {
		if (cmdline->argv[i])
			co_os_free(cmdline->argv[i]);
	}
	co_os_free(cmdline->argv);
	co_os_free(cmdline);
}

void co_remove_quotation_marks(char *value)
{
	int length;

	if (*value == '\"') {
		length = co_strlen(value)-1;
		if (value[length] == '\"') {
			length--;
			co_memcpy(value, value+1, length);
			value[length] = '\0';
		}
	}
}
