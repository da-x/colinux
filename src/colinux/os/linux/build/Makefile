# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..

all: colinux

include $(BUILD_ROOT)/Makefile
else

BUILD_PATH := $(BUILD_PATH)/build
#-----------------------------------------------------------------

LOCAL_FILES = \
	$(BUILD_ROOT)/colinux/os/current/user/console/colinux-console-fltk \
	$(BUILD_ROOT)/colinux/os/current/user/daemon/colinux-daemon \
	$(BUILD_ROOT)/colinux/os/current/user/conet-daemon/colinux-net-daemon \
	$(BUILD_ROOT)/colinux/os/current/user/coserial-daemon/colinux-serial-daemon \

LOCAL_TARGETS := $(LOCAL_TARGET) $(BUILD_PATH)/driver.o

colinux: $(LOCAL_FILES) kernel_module

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
