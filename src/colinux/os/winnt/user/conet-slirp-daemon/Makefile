# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/conet-slirp-daemon
#-----------------------------------------------------------------

LOCAL_EXE = $(BUILD_PATH)/colinux-slirp-net-daemon.exe
LOCAL_OBJS = \
	main.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c 
	$(SLIRP_TOOL_USER_CC) $(HOST_API_CFLAGS)

LOCAL_TARGETS = $(LOCAL_OBJS) $(LOCAL_EXE)

$(LOCAL_EXE) : \
	$(COLINUX_WINNT_ICON) \
	$(LOCAL_OBJS) \
	$(BUILD_ROOT)/colinux/os/current/user/file.o \
	$(BUILD_ROOT)/colinux/user/reactor.o \
	$(BUILD_ROOT)/colinux/user/slirp/slirp.a \
	$(BUILD_ROOT)/colinux/os/current/user/user.a \
	$(BUILD_ROOT)/colinux/user/user.a \
	$(BUILD_ROOT)/colinux/common/common.a \
	$(BUILD_ROOT)/colinux/os/current/user/manager.o \
	$(BUILD_ROOT)/colinux/os/current/user/timer.o \

	$(CC) $^ -o $@ -luser32 -lgdi32 -lkernel32 -lmxml -lws2_32 \
	-liphlpapi

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_EXE)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
