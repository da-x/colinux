# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/console-nt
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-console-nt.exe

LOCAL_OBJS = \
	head.o \
	widget.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.cpp 
	$(TOOL_USER_GPP) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(BUILD_ROOT)/colinux/user/console-base/user-console.o \
	$(BUILD_ROOT)/colinux/os/current/user/user.o \
	$(LOCAL_OBJS) \
	$(BUILD_ROOT)/colinux/common/common.a \

	$(GPP) $^ -o $@ $(USER_DEFAULT_LIBS) 

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
