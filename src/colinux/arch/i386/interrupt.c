/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#include <colinux/arch/interrupt.h>

#define INTERRUPT_GATE_PROXY \
	INTERRUPT_GATE_PROXY_9(0)		\
	INTERRUPT_GATE_PROXY_9(128)

#define INTERRUPT_GATE_PROXY_9(index)		\
	INTERRUPT_GATE_PROXY_8(index)		\
	INTERRUPT_GATE_PROXY_8(index+64)

#define INTERRUPT_GATE_PROXY_8(index)		\
	INTERRUPT_GATE_PROXY_7(index)		\
	INTERRUPT_GATE_PROXY_7(index+32)

#define INTERRUPT_GATE_PROXY_7(index)		\
	INTERRUPT_GATE_PROXY_6(index)		\
	INTERRUPT_GATE_PROXY_6(index+16)

#define INTERRUPT_GATE_PROXY_6(index)		\
	INTERRUPT_GATE_PROXY_5(index)		\
	INTERRUPT_GATE_PROXY_5(index+8)

#define INTERRUPT_GATE_PROXY_5(index)		\
	INTERRUPT_GATE_PROXY_4(index)		\
	INTERRUPT_GATE_PROXY_4(index+4)

#define INTERRUPT_GATE_PROXY_4(index)		\
	INTERRUPT_GATE_PROXY_3(index)		\
	INTERRUPT_GATE_PROXY_3(index+2)

#define INTERRUPT_GATE_PROXY_3(index)		\
	INTERRUPT_GATE_PROXY_2(index)		\
	INTERRUPT_GATE_PROXY_2(index+1)

#define INTERRUPT_GATE_PROXY_2(index)		\
	{0xcd, index, 0xfb, 0xc3},
/* 
 *  The assembly code above is:
 * 
 *  int index;
 *  sti;
 *  ret;
 */ 

static unsigned char interrupt_recall_array[0x100][4] = {
	INTERRUPT_GATE_PROXY
};

void co_monitor_arch_real_hardware_interrupt(co_monitor_t *cmon)
{
	/* Raise the interrupt */
	unsigned char *forward_thunk;

	forward_thunk = (unsigned char *)&interrupt_recall_array[co_passage_page->params[0]];
	
	((void (*)())forward_thunk)();
}
