#include <colinux/os/alloc.h>

#include "osdep.h"

/*
 * Windows parameters are one long whitespace separated strings.
 * This function splits them to normal NULL-terminated strings.
 */
co_rc_t co_os_parse_args(LPSTR szCmdLine, int *count, char ***args)
{
	char *param_scan;
	long param_count = 0, i, j;
	char **param_array;

	param_scan = szCmdLine;
	while (*param_scan != '\0') {
		while (*param_scan == ' ')
			param_scan++;

		if (*param_scan == '\0')
			break;

		for (;;param_scan++) {
			if (*param_scan == '"') {
				param_scan++;
				while (*param_scan != '"'  &&  *param_scan != '\0') {
					if (*param_scan == '\\')
						param_scan++;
					if (*param_scan == '\0')
						return CO_RC(ERROR);
					param_scan++;
				}
				if (*param_scan == '\0')
					break;
				param_scan++;
			}
			
			if (*param_scan == '\0')
				break;
			
			if (*param_scan == ' ')
				break;
		}
		param_count++;
	}

	param_array = (char **)co_os_malloc(sizeof(char *)*param_count + 1);
	if (param_array == NULL) {
		free(param_array);
		return CO_RC(ERROR);
	}
	memset(param_array, 0, sizeof(char *)*param_count + 1);

	i = 0;
	param_scan = szCmdLine;
	while (*param_scan != '\0') {
		int size = 0;

		while (*param_scan == ' ')
			param_scan++;

		if (*param_scan == '\0')
			break;

		for (;;param_scan++) {
			if (*param_scan == '"') {
				param_scan++;
				while (*param_scan != '"'  &&  *param_scan != '\0') {
					if (*param_scan == '\\')
						param_scan++;
					if (*param_scan == '\0')
						goto error;

					if ((param_scan[-1] == '\\')  &&  
					    (*param_scan != '"'))
						size++;

					size++;
					param_scan++;
				}
				if (*param_scan == '\0')
					break;
				param_scan++;
			}
			
			if (*param_scan == '\0')
				break;
			
			if (*param_scan == ' ')
				break;

			size++;
		}

		param_array[i] = co_os_malloc(size + 1);
		if (!param_array[i])
			goto error;
		i++;
	}

	i = 0;
	param_scan = szCmdLine;
	while (*param_scan != '\0') {
		while (*param_scan == ' ')
			param_scan++;

		if (*param_scan == '\0')
			break;

		j = 0;
		for (;;param_scan++) {
			if (*param_scan == '"') {
				param_scan++;
				while (*param_scan != '"'  &&  *param_scan != '\0') {
					int slash = 0;
					if (*param_scan == '\\') {
						param_scan++;
						slash = 1;
					}
					if (*param_scan == '\0')
						goto error;

					if (slash && (*param_scan != '"')) {
						param_array[i][j] = '\\';
						j++;
					}

					param_array[i][j] = *param_scan;
					j++;
					param_scan++;
				}
				if (*param_scan == '\0')
					break;
				param_scan++;
			}
			
			if (*param_scan == '\0')
				break;
			
			if (*param_scan == ' ')
				break;

			param_array[i][j] = *param_scan;
			j++;
		}

		param_array[i][j] = '\0';

		i++;
	}
	
	*args = param_array;
	*count = param_count;

	return CO_RC(OK);

error:
	for (i=0; i < param_count; i++)
		if (param_array[i])
			co_os_free(param_array[i]);

	co_os_free(param_array);

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
