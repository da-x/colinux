# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/kernel
#-----------------------------------------------------------------

LOCAL_OBJS = \
	core.o \
	block.o \
	interface.o \
	manager.o \
	alloc.o \
	filesystem.o \
	fileio.o \
	time.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC) $(HOST_API_CFLAGS)

$(BUILD_PATH)/kernel.o : $(LOCAL_OBJS) $(BUILD_PATH)/lowlevel/lowlevel.o
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/kernel.o

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, lowlevel)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
