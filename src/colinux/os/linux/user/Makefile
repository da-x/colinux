# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/user
#-----------------------------------------------------------------

LOCAL_OBJS = \
	monitor.o \
	manager.o \
	alloc.o \
	misc.o \
	file.o \
	pipe.o \
	exec.o \
	daemon.o \
	unix.o \
	frame.o \
	cobdpath.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_USER_CC)

$(BUILD_PATH)/user.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user.a

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, daemon console conet-daemon coserial-daemon debug)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
