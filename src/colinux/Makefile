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

ifneq ($(COLINUX_NDEBUG),Y)
CFLAGS += -DCOLINUX_DEBUG
endif

ifeq ($(COLINUX_DEBUG_SYMBOLS),Y)
CFLAGS += -g -O0
else
CFLAGS += -O2
endif

# Add additional USER_CLAGS for special-case SLIRP compile:
SLIRP_USER_CFLAGS= $(filter-out '-O -O0 -O1 -O2 -O3 -O4 -O5', $(USER_CFLAGS)) -O

export COLINUX_NDEBUG

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, arch kernel os user common)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
