#include <colinux/os/alloc.h>

#include "osdep.h"

/*
 * Windows parameters are one long whitespace separated strings.
 * This function splits them to normal NULL-terminated strings.
 */
co_rc_t co_os_parse_args(LPSTR szCmdLine, int *count, char ***args)
{
	char *param_scan;
	long param_count = 0, i;
	char **param_array;

	param_scan = szCmdLine;

	while (*param_scan == ' ')
		param_scan++;

	for (;;) {
		unsigned long param_advance = 0;
		if (*param_scan == '"') {
			param_scan++;
			while (*param_scan != '"' &&  *param_scan) {
				param_scan++;
				param_advance++;
			}
			if (*param_scan == '"')
				param_scan++;
		}
		else {
			while (*param_scan != ' ' &&  *param_scan) {
				param_scan++;
				param_advance++;
			}
		}

		if (param_advance != 0)
			param_count++;

		if (*param_scan == '\0')
			break;
	
		while (*param_scan == ' ')
			param_scan++;

		if (*param_scan == '\0')
			break;
	}

	param_array = (char **)co_os_malloc(sizeof(char *)*param_count + 1);
	if (param_array == NULL) {
		free(param_array);
		return CO_RC(ERROR);
	}

	param_scan = szCmdLine;
	while (*param_scan == ' ')
		param_scan++;

	for (i = 0; i < param_count; i++) {
		unsigned long param_advance = 0;
		char *param_start = param_scan;

		if (*param_scan == '\0')
			break;

		if (*param_scan == '"') {
			param_scan++;
			param_start++;
			while (*param_scan != '"' &&  *param_scan) {
				param_scan++;
				param_advance++;
			}
			if (*param_scan == '"')
				param_scan++;
		}
		else {
			while (*param_scan != ' '  &&  *param_scan) {
				param_scan++;
				param_advance++;
			}
		}
		
		if (param_scan != param_start) {
			param_array[i] = co_os_malloc(param_advance+1);
			if (param_array[i] == NULL)
				break;
				
			memcpy(param_array[i], param_start, param_advance);
			param_array[i][param_advance] = '\0';
		}

		while (*param_scan == ' ')
			param_scan++;
	}

	if (i == param_count) {
		*count = param_count;
		param_array[param_count] = NULL;
		*args = param_array;
		return CO_RC(OK);
	} else {
		/* Error path */

		int j;
		for (j=0; j < i; j++)
			free(param_array[j]);
	}
	free(param_array);

	return CO_RC(ERROR);
}

void co_os_free_parsed_args(char **args)
{
	char **param_scan;

	param_scan = args;
	
	while (*param_scan) {
		co_os_free(*param_scan);
		param_scan++;
	}

	co_os_free(args);
}
