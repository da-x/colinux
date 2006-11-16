# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C .. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/colinux
#-----------------------------------------------------------------

CFLAGS = -DCOLINUX -DCOLINUX_ARCH=$(COLINUX_HOST_OS) \
	-mpush-args -mno-accumulate-outgoing-args \
	-Wno-trigraphs -fno-strict-aliasing -Wall \
	$(COLINUX_CFLAGS) \

ifneq ($(COLINUX_NDEBUG),Y)
CFLAGS += -DCOLINUX_DEBUG
endif

ifeq ($(COLINUX_DEBUG_SYMBOLS),Y)
CFLAGS += -g -O0
else
CFLAGS += -O2
endif

export COLINUX_NDEBUG

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, arch kernel os user common)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
