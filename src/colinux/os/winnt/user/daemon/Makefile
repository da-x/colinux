# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/daemon
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-daemon.exe

LOCAL_OBJS = \
	main.o \
	win32tap.o \
	network.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(BUILD_ROOT)/colinux/common/common.o \
	$(BUILD_ROOT)/colinux/user/user.o \
	$(BUILD_ROOT)/colinux/os/current/user/user.o \
	$(LOCAL_OBJS)

	$(CC) $^ -o $@ -luser32 -lgdi32 -lkernel32 -lmxml

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
