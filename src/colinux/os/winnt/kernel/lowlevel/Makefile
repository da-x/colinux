# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/lowlevel
#-----------------------------------------------------------------

LOCAL_OBJS = \
	timer.o \
	time.o \
	mutex.o \
	misc.o \
	debug.o \
	alloc.o \
	wait.o \
	user.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC) $(HOST_API_CFLAGS)

$(BUILD_PATH)/lowlevel.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/lowlevel.o

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
