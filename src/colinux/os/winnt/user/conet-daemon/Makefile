# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/conet-daemon
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-net-daemon.exe

LOCAL_OBJS = \
	tap-win32.o \

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
	$(LOCAL_OBJS_CPP) $(LOCAL_OBJS)  \
	$(BUILD_ROOT)/colinux/user/daemon-base/daemon-base.o \
	$(BUILD_ROOT)/colinux/user/reactor.o \
	$(BUILD_ROOT)/colinux/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \

	$(GPP) $^ -o $@ -luser32 -lgdi32 -lkernel32

CLEAN_FILES := $(CLEAN_FILES) $(LOCAL_TARGETS)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
