# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/common
#-----------------------------------------------------------------

LOCAL_OBJS = \
	queue.o \
	console.o \
	debug.o \
	messages.o \
	version.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC)

$(BUILD_PATH)/version.h : $(BUILD_PATH)/version.py $(BUILD_PATH)/../VERSION
	python $(word 1,$^) $(word 2,$^) $@

$(BUILD_PATH)/version.o : $(BUILD_PATH)/version.h

$(BUILD_PATH)/common.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/common.a \
	$(BUILD_PATH)/version.h

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
