#ifndef __CO_LINUX_IOCTL_H__
#define __CO_LINUX_IOCTL_H__

typedef struct co_linux_io {
	unsigned long code;
	void *input_buffer;
	unsigned long input_buffer_size;
	void *output_buffer;
	unsigned long output_buffer_size;
	unsigned long *output_returned;
} co_linux_io_t;

#define CO_LINUX_IOCTL_ID  0x12340000

#endif
