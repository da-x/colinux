# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../.. $(MAKECMDGOALS)
else

BUILD_PATH := $(BUILD_PATH)/kernel
#-----------------------------------------------------------------

LOCAL_TARGETS = $(BUILD_PATH)/driver.o
LOCAL_OBJS = \
	core.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC) $(HOST_API_CFLAGS)

$(BUILD_PATH)/kernel.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

$(BUILD_PATH)/module/prelinked_driver.o : \
	$(BUILD_ROOT)/colinux/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/arch/current/arch.o \
	$(BUILD_ROOT)/colinux/common/common.a \
	$(BUILD_PATH)/kernel.o

	$(TOOL_LD_RELOC)

prelinked_driver: $(BUILD_PATH)/module/prelinked_driver.o

$(BUILD_PATH)/module/colinux.ko: $(BUILD_PATH)/module $(BUILD_PATH)/module/prelinked_driver.o
	(export COLINUX_PWD=`pwd` ; cd $(word 1, $^) ; make V=1)

kernel_module: $(BUILD_PATH)/module/colinux.ko

LOCAL_TARGETS := $(BUILD_PATH)/prelinked_driver.o

INSTALL_FILES := $(INSTALL_FILES) \
	$(BUILD_PATH)/module/colinux.ko

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(LOCAL_TARGETS) \
	$(BUILD_PATH)/module/{*.o,.*.ko.cmd,.*.o.cmd,*.ko,*.mod.c,.tmp_versions/*}

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif

