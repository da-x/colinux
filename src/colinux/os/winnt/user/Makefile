# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/user
#-----------------------------------------------------------------

LOCAL_OBJS = \
	manager.o \
	file.o \
	alloc.o \
	debug.o \
	misc.o \
	osdep.o \
	timer.o \
	pipe.o \
	daemon.o \
	exec.o \


LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

$(BUILD_PATH)/user.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user.o

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, daemon conet-daemon console conet-bridged-daemon)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
