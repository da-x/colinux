# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/conet-daemon
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-net-daemon
USER_CFLAGS += $(CFLAGS) 

LOCAL_OBJS = \
	tap.o \

LOCAL_OBJS_CPP = \
	daemon.o 

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))
LOCAL_OBJS_CPP := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS_CPP))

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

$(LOCAL_OBJS_CPP) : %.o : %.cpp
	$(TOOL_USER_GPP) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_OBJS_CPP) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(LOCAL_OBJS_CPP) $(LOCAL_OBJS) \
	$(BUILD_ROOT)/colinux/user/daemon-base/daemon-base.o \
	$(BUILD_ROOT)/colinux/user/user.a \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \

	$(GPP) $^ -o $@ 

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS_CPP) $(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif