# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/colinux
#-----------------------------------------------------------------

CFLAGS = \
	-mpush-args -mno-accumulate-outgoing-args \
        -Wno-trigraphs -O2 -fno-strict-aliasing -Wall \

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, arch kernel os user common)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
