#ifndef __CO_WINNT_IOCTL_H__
#define __CO_WINNT_IOCTL_H__

#define CO_WINNT_IOCTL(_method_) \
    CTL_CODE(CO_DRIVER_TYPE, _method_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define CO_GET_IOCTL_TYPE(ioctl)     ((ioctl >> 16) & ((1 << 16) - 1))
#define CO_GET_IOCTL_ACCESS(ioctl)   ((ioctl >> 14) & ((1 << 2) - 1))
#define CO_GET_IOCTL_METHOD(ioctl)   ((ioctl >> 2) & ((1 << 12) - 1))
#define CO_GET_IOCTL_MTYPE(ioctl)    ((ioctl >> 0) & ((1 << 2) - 1))

typedef enum {
	COKC_SYSDEP_SET_NETDEV,
} cokc_sysdep_ioctl_op_t;

typedef struct {
	cokc_sysdep_ioctl_op_t op;
	union {
		struct { /* COKC_SYSDEP_SET_NETDEV */
			unsigned char pathname[0x200];
		} set_netdev;
	};
} cokc_sysdep_ioctl_t;

#endif
