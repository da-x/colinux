# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/user
#-----------------------------------------------------------------

USER_CFLAGS = $(CFLAGS) 
# -DDEBUG_CO_RC

LOCAL_OBJS = \
	monitor.o \
	manager.o \
	config.o \
	elf_load.o \
	daemon.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_USER_CC)

$(BUILD_PATH)/user.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user.o 

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, console)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
