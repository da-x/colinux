/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 *
 */

#ifndef __CO_OS_KERNEL_TIME_H__
#define __CO_OS_KERNEL_TIME_H__

/*
 * Obtains the GMT time_t from the host.
 */

extern unsigned long co_os_get_time();

/*
 * Return 2^32 * (1 / (TSC clocks per usec)) for do_fast_gettimeoffset().
 */
extern unsigned long co_os_get_high_prec_quotient();

#endif

