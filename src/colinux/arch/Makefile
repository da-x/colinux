# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/arch
#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, current)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
