# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..
include $(BUILD_ROOT)/Makefile
else

BUILD_PATH := $(BUILD_PATH)/kernel
#-----------------------------------------------------------------

LOCAL_TARGETS = $(BUILD_PATH)/driver.o

$(BUILD_PATH)/module/prelinked_driver.o : \
	$(BUILD_ROOT)/colinux/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/arch/current/arch.o \
	$(BUILD_ROOT)/colinux/common/common.a \
	
	$(TOOL_LD_RELOC)

prelinked_driver: $(BUILD_PATH)/module/prelinked_driver.o

$(BUILD_PATH)/module/colinux.ko: $(BUILD_PATH)/module $(BUILD_PATH)/module/prelinked_driver.o
	(cd $(word 1, $^) ; make COLINUX_PWD=$(BUILD_ROOT) COLINUX_PWD=`pwd`/../../../../..)

kernel_module: $(BUILD_PATH)/module/colinux.ko

LOCAL_TARGETS := $(BUILD_PATH)/prelinked_driver.o

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS) $(BUILD_PATH)/module/*.o

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
