# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..

all: colinux

include $(BUILD_ROOT)/Makefile
else

BUILD_PATH := $(BUILD_PATH)/build
#-----------------------------------------------------------------

LOCAL_FILES_PATH = $(BUILD_ROOT)/colinux/os/current/user
LOCAL_FILES := \
	$(LOCAL_FILES_PATH)/console/colinux-console-fltk \
	$(LOCAL_FILES_PATH)/daemon/colinux-daemon \
	$(LOCAL_FILES_PATH)/debug/colinux-debug-daemon \
	$(LOCAL_FILES_PATH)/conet-daemon/colinux-net-daemon \
	$(LOCAL_FILES_PATH)/conet-slirp-daemon/colinux-slirp-net-daemon \
	$(LOCAL_FILES_PATH)/coserial-daemon/colinux-serial-daemon \

LOCAL_TARGETS := $(LOCAL_TARGET) $(BUILD_PATH)/driver.o

colinux: $(LOCAL_FILES) kernel_module

INSTALL_FILES := $(INSTALL_FILES) \
	$(LOCAL_FILES)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
