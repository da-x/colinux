# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/current
#-----------------------------------------------------------------

ifeq ($(GCCTRACE),Y)
CC=$(BUILD_ROOT)/../bin/tracewrapper.py $(CROSS_TARGET)gcc
else
CC=$(CROSS_TARGET)gcc
endif
GPP=$(CROSS_TARGET)g++
LD=$(CROSS_TARGET)ld

USER_CFLAGS += -g
KERNEL_CFLAGS += -g
 
#-----------------------------------------------------------------
include $(call BUILD_SUBDIRS, kernel user build test)
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
