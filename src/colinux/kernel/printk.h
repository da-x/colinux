
#ifndef __COOPERATIVE_DRIVER_PRINTK_H
#define __COOPERATIVE_DRIVER_PRINTK_H

#include <stdarg.h>

#include "monitor.h"

extern void printk(co_monitor_t *, char *, ...);

#endif
