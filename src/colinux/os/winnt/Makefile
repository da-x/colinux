# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/current
#-----------------------------------------------------------------

ifeq ($(COLINUX_ARCH),i386)
CROSS_TARGET=i686-pc-mingw32-
else
$(error Other Windows architectures not supported yet!)
endif

CFLAGS_WINVER = 0x0500
USER_CFLAGS += -DWINVER=$(CFLAGS_WINVER)
KERNEL_CFLAGS += -DWINVER=$(CFLAGS_WINVER)

ifeq ($(GCCTRACE),Y)
CC=$(BUILD_ROOT)/../bin/tracewrapper.py $(CROSS_TARGET)gcc
else
CC=$(CROSS_TARGET)gcc
endif
GPP=$(CROSS_TARGET)g++
LD=$(CROSS_TARGET)ld

USER_DEFAULT_LIBS = \
     -mno-cygwin -luser32 -lgdi32 -lws2_32 \
     -lkernel32 -lole32 -luuid -lgdi32 -lmsvcrt -lcrtdll -lshlwapi

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, build kernel user)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
