/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <malloc.h>
#include "cmdline.h"

#include <colinux/os/alloc.h>
#include <colinux/os/user/misc.h>
#include <colinux/common/libc.h>

struct co_command_line_params {
	int argc;
	char **argv;
};

co_rc_t co_cmdline_params_alloc(char **argv, int argc, co_command_line_params_t *cmdline_out)
{
	co_command_line_params_t cmdline;
	unsigned long argv_size;

	cmdline = (typeof(cmdline))(malloc(sizeof(struct co_command_line_params)));
	if (cmdline == NULL)
		return CO_RC(ERROR);

	argv_size = (sizeof(char *))*(argc+1); /* To avoid allocations */
	cmdline->argv = (char **)malloc(argv_size);
	cmdline->argc = argc;
	co_memcpy(cmdline->argv, argv, (sizeof(char*))*argc);

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
			cmdline->argv[i] = NULL;
			*out_exists = PTRUE;
			return CO_RC(OK);
		}
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_get_next_equality(co_command_line_params_t cmdline, const char *expected_prefix, int max_suffix_len,
				     char *key, int key_size, char *value, int value_size, bool_t *out_exists)
{
	int i;
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
		
		if (!!co_strncmp(expected_prefix, cmdline->argv[i], prefix_len))
			continue;
		
		cmdline->argv[i] = NULL;
		
		key_len = (key_found - arg) - prefix_len;
		if (key_len > 0) {
			if (key_size < key_len + 1)
				return CO_RC(ERROR);
			
			co_memcpy(key, prefix_len + arg, key_len);
			key[key_len] = '\0';
		}
		
		if (co_strlen(&key_found[1]) + 1 > value_size)
			return CO_RC(ERROR);
		
		co_snprintf(value, value_size, "%s", &key_found[1]);
		
		*out_exists = PTRUE;
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_get_next_equality_int_prefix(co_command_line_params_t cmdline, const char *expected_prefix, 
						int *key_int, char *value, int value_size, bool_t *out_exists)
{
	char number[11];
	co_rc_t rc;
	
	rc = co_cmdline_get_next_equality(cmdline, expected_prefix, sizeof(number) - 1,
					  number, sizeof(number), value, value_size, out_exists);
	if (!CO_OK(rc)) 
		return rc;
	
	if (*out_exists) {
		char *number_parse  = NULL;
		int value_int;
		
		value_int = co_strtol(number, &number_parse, 10);
		if (number_parse == number) {
			/* not a number */
			return CO_RC(ERROR);
		}

		*key_int = value_int;
	}

	return CO_RC(OK);
}

co_rc_t co_cmdline_get_next_equality_int_value(co_command_line_params_t cmdline, const char *expected_prefix, 
					       int *value_int, bool_t *out_exists)
{
	char value[0x100];
	co_rc_t rc;
	
	rc = co_cmdline_get_next_equality(cmdline, expected_prefix, 0, NULL, 0, value, sizeof(value), out_exists);
	if (!CO_OK(rc)) 
		return rc;
	
	if (*out_exists) {
		char *value_parse = NULL;
		
		*value_int = co_strtol(value, &value_parse, 10);
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
			cmdline->argv[i+1] = NULL;
		}
		
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
						     bool_t *out_exists, int *out_int)
{
	char arg_buf[0x20];
	co_rc_t rc;
	char *end_ptr;

	rc = co_cmdline_params_one_arugment_parameter(cmdline, name, out_exists, arg_buf, sizeof(arg_buf));
	if (!CO_OK(rc))
		return rc;

	if (out_exists && *out_exists) { 
		*out_int = co_strtol(arg_buf, &end_ptr, 10);
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

		cmdline->argv[i] = NULL;
	}

	return CO_RC(OK);
}

void co_cmdline_params_free(co_command_line_params_t cmdline)
{
	co_os_free(cmdline->argv);
	co_os_free(cmdline);
}
