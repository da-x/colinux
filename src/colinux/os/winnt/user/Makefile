# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/user
#-----------------------------------------------------------------

LOCAL_OBJS = \
	manager.o \
	file.o \
	alloc.o \
	misc.o \
	osdep.o \
	timer.o \
	exec.o \
	udp.o \
	cobdpath.o \
	reactor.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

COLINUX_WINNT_ICON = $(BUILD_ROOT)/colinux/os/current/user/daemon/res/daemon.res

$(LOCAL_OBJS) : %.o : %.c 
	$(TOOL_USER_CC) $(HOST_API_CFLAGS)

$(BUILD_PATH)/user.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user.a

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, daemon conet-daemon console \
	console-nt conet-bridged-daemon coserial-daemon \
	conet-slirp-daemon install debug control)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
