/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2004 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_USER_CMDLINE_H__
#define __COLINUX_USER_CMDLINE_H__

#include <colinux/common/common.h>

typedef struct co_command_line_params *co_command_line_params_t;

extern co_rc_t co_cmdline_params_alloc(char **argv, int argc, co_command_line_params_t *cmdline_out);
extern co_rc_t co_cmdline_params_argumentless_parameter(co_command_line_params_t cmdline, 
							const char *name,
							bool_t *out_exists);
extern co_rc_t co_cmdline_params_one_arugment_parameter(co_command_line_params_t cmdline, 
							const char *name, 
							bool_t *out_exists, 
							char *out_arg_buf, int arg_size);
extern co_rc_t co_cmdline_params_one_optional_arugment_parameter(co_command_line_params_t cmdline, 
								 const char *name, 
								 bool_t *out_exists, 
								 char *out_arg_buf, int arg_size);
extern void co_cmdline_params_free(co_command_line_params_t cmdline);

extern co_rc_t co_cmdline_params_check_for_no_unparsed_parameters(co_command_line_params_t cmdline,
								  bool_t print);

#endif
