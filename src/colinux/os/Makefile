# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/os
#-----------------------------------------------------------------

#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, current)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
