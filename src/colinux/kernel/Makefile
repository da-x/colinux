# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/kernel
#-----------------------------------------------------------------

KERNEL_CFLAGS = $(CFLAGS) -D__KERNEL__ -DCO_KERNEL -DCO_HOST_KERNEL

LOCAL_OBJS = \
	monitor.o \
	block.o \
	fileblock.o \
	transfer.o \
	manager.o \
	pages.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC)

$(BUILD_PATH)/kernel.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/kernel.o

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
