# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/console
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-console.exe

LOCAL_OBJS = \
	head.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.cpp 
	$(TOOL_USER_GPP) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(BUILD_ROOT)/colinux/common/common.o \
	$(BUILD_ROOT)/colinux/user/user.o \
	$(BUILD_ROOT)/colinux/user/console/user-console.o \
	$(BUILD_ROOT)/colinux/os/current/user/user.o \
	$(LOCAL_OBJS)

	$(GPP) $^ -o $@ -luser32 -lgdi32 -lkernel32 -lfltk \
        -lole32 -luuid -lgdi32 -lmxml -Wl,--subsystem,windows

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
