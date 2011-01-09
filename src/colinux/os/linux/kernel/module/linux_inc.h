#include <linux/version.h>
#include <linux/module.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#endif
#include <linux/compiler.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
