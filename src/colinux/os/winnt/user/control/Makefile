# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/control
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux.exe
USER_CFLAGS += $(CFLAGS) 

LOCAL_OBJS = \
	main.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(BUILD_PATH)/res/daemon.res: $(BUILD_PATH)/res $(BUILD_PATH)/res/daemon.rc
	$(CROSS_TARGET)windres -O coff -I $(word 1, $^) $(word 2, $^) -o $@

$(LOCAL_EXE) : \
	$(LOCAL_OBJS) \
	$(BUILD_ROOT)/colinux/user/reactor.o \
	$(BUILD_ROOT)/colinux/common/debug.o \
	$(BUILD_ROOT)/colinux/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \

	$(CC) $^ -o $@ -lmxml $(USER_DEFAULT_LIBS)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
