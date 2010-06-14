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

extern co_rc_t co_cmdline_params_one_arugment_int_parameter(co_command_line_params_t cmdline,
							    const char *name,
							    bool_t *out_exists,
							    unsigned int *out_int);

extern co_rc_t co_cmdline_params_one_optional_arugment_parameter(co_command_line_params_t cmdline,
								 const char *name,
								 bool_t *out_exists,
								 char *out_arg_buf, int arg_size);

extern co_rc_t co_cmdline_get_next_equality(co_command_line_params_t cmdline, const char *expected_prefix,
					    int max_suffix_len, char *key, int key_size, char *value,
					    int value_size, bool_t *out_exists);

extern co_rc_t co_cmdline_get_next_equality_alloc(co_command_line_params_t cmdline, const char *expected_prefix,
					    int max_suffix_len, char *key, int key_size, char **pp_value,
					    bool_t *out_exists);

extern co_rc_t co_cmdline_get_next_equality_int_prefix(co_command_line_params_t cmdline, const char *expected_prefix,
						       unsigned int *key_int, unsigned int max_index,
						       char **value, bool_t *out_exists);

extern co_rc_t co_cmdline_get_next_equality_int_value(co_command_line_params_t cmdline, const char *expected_prefix,
						      unsigned int *value_int, bool_t *out_exists);

extern void co_cmdline_params_free(co_command_line_params_t cmdline);

extern co_rc_t co_cmdline_params_check_for_no_unparsed_parameters(co_command_line_params_t cmdline,
								  bool_t print);
extern co_rc_t co_cmdline_params_format_remaining_parameters(co_command_line_params_t cmdline,
							     char *str_out, int size);

extern void co_remove_quotation_marks(char *value);

#endif
