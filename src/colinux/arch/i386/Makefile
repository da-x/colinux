# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/current
#-----------------------------------------------------------------

LOCAL_OBJS = \
	passage.o \
	cpuid.o \
	interrupt.o \
	manager.o \
	apic.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC)

$(BUILD_PATH)/arch.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/arch.o

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
