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
	memcpy(cmdline->argv, argv, (sizeof(char*))*argc);

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

		if (strcmp(cmdline->argv[i], name) == 0) {
			cmdline->argv[i] = NULL;
			*out_exists = PTRUE;
			return CO_RC(OK);
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

		if (strcmp(cmdline->argv[i], name) != 0)
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
		*out_int = strtol(arg_buf, &end_ptr, 10);
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

void co_cmdline_params_free(co_command_line_params_t cmdline)
{
	co_os_free(cmdline->argv);
	co_os_free(cmdline);
}
