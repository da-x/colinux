# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/coserial-daemon
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-serial-daemon.exe
LOCAL_OBJS = main.o 

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(LOCAL_OBJS) \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \
	$(BUILD_ROOT)/colinux/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \
	$(BUILD_ROOT)/colinux/os/current/user/manager.o \
	$(BUILD_ROOT)/colinux/os/current/user/timer.o \

	$(CC) $^ -o $@ 

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif