# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/user
#-----------------------------------------------------------------

# -DDEBUG_CO_RC

LOCAL_OBJS = \
	monitor.o \
	manager.o \
	config.o \
	elf_load.o \
	macaddress.o \
	daemon.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_USER_CC)

$(BUILD_PATH)/daemon.o : $(BUILD_ROOT)/colinux/common/version.h

$(BUILD_PATH)/user.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user.a

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, console-base console)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
