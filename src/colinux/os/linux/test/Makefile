# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/test
#-----------------------------------------------------------------

LOCAL_EXE1 = $(BUILD_PATH)/pipe_server
LOCAL_OBJS1 = pipe_server.o

USER_CFLAGS += $(CFLAGS) 

LOCAL_OBJS1 := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS1))
LOCAL_OBJS := \
	$(LOCAL_OBJS1) \

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE1)

$(LOCAL_EXE1) : \
	$(LOCAL_OBJS1) \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \

	$(CC) $^ -o $@

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
