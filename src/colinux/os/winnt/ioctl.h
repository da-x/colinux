#ifndef __CO_WINNT_IOCTL_H__
#define __CO_WINNT_IOCTL_H__

#ifdef WIN64
#define CO_WINNT_IOCTL_64BIT_FLAG  0x800 /* bit in opcode represent 64 bit user land access */
#define CO_WINNT_IOCTL(_method_) \
    CTL_CODE(CO_DRIVER_TYPE, _method_ | CO_WINNT_IOCTL_64BIT_FLAG, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CO_WINNT_IOCTL_MASK64(a) (a | (CO_WINNT_IOCTL_64BIT_FLAG << 2))
#else /* WIN64 */
#define CO_WINNT_IOCTL(_method_) \
    CTL_CODE(CO_DRIVER_TYPE, _method_, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CO_WINNT_IOCTL_MASK64
#endif /* WIN64 */

#define CO_GET_IOCTL_TYPE(ioctl)     ((ioctl >> 16) & ((1 << 16) - 1))
#define CO_GET_IOCTL_ACCESS(ioctl)   ((ioctl >> 14) & ((1 << 2) - 1))
#define CO_GET_IOCTL_METHOD(ioctl)   ((ioctl >> 2) & ((1 << 12) - 1))
#define CO_GET_IOCTL_MTYPE(ioctl)    ((ioctl >> 0) & ((1 << 2) - 1))

#endif
