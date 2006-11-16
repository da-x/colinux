# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../.. $(MAKECMDGOALS)
all: colinux
else

BUILD_PATH := $(BUILD_PATH)/build
#-----------------------------------------------------------------

LOCAL_TARGETS = $(BUILD_PATH)/driver.o

$(BUILD_PATH)/driver.o : \
	$(BUILD_ROOT)/colinux/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/os/current/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/arch/current/arch.o \
	$(BUILD_ROOT)/colinux/common/common.a \

	$(TOOL_LD_RELOC)

DRIVER_TARGET := $(BUILD_PATH)/linux.sys

CLEAN_FILES += $(BUILD_PATH)/driver.base.tmp
$(BUILD_PATH)/driver.base.tmp: $(BUILD_PATH)/driver.o
	$(CROSS_TARGET)gcc -Wl,--base-file,$@ \
	-Wl,--entry,_DriverEntry@8 \
	-nostartfiles -nostdlib \
        -o junk.tmp $^ -lntoskrnl -lhal -lgcc
	rm -f junk.tmp

CLEAN_FILES += $(BUILD_PATH)/driver.base.exp
$(BUILD_PATH)/driver.base.exp: $(BUILD_PATH)/driver.base.tmp
	$(CROSS_TARGET)dlltool --dllname linux.sys \
	--base-file $^  \
	--output-exp $@

$(DRIVER_TARGET): $(BUILD_PATH)/driver.o $(BUILD_PATH)/driver.base.exp
	$(CROSS_TARGET)gcc  \
        -Wl,--subsystem,native \
        -Wl,--image-base,0x10000 \
        -Wl,--file-alignment,0x1000 \
        -Wl,--section-alignment,0x1000 \
        -Wl,--entry,_DriverEntry@8 \
        -Wl,$(word 2,$^) \
        -mdll -nostartfiles -nostdlib \
        -o $@ $(word 1,$^) -lntoskrnl -lhal -lgcc

LOCAL_TARGET += $(DRIVER_TARGET)
LOCAL_FILES_PATH = $(BUILD_ROOT)/colinux/os/current/user
LOCAL_FILES = \
	$(LOCAL_FILES_PATH)/daemon/colinux-daemon.exe \
	$(LOCAL_FILES_PATH)/console/colinux-console-fltk.exe \
	$(LOCAL_FILES_PATH)/debug/colinux-debug-daemon.exe \
	$(LOCAL_FILES_PATH)/console-nt/colinux-console-nt.exe \
	$(LOCAL_FILES_PATH)/coserial-daemon/colinux-serial-daemon.exe \
	$(LOCAL_FILES_PATH)/conet-daemon/colinux-net-daemon.exe \
	$(LOCAL_FILES_PATH)/conet-bridged-daemon/colinux-bridged-net-daemon.exe \
	$(LOCAL_FILES_PATH)/conet-slirp-daemon/colinux-slirp-net-daemon.exe \
	$(DRIVER_TARGET) \

LOCAL_TARGETS := $(LOCAL_TARGET) $(BUILD_PATH)/driver.o

bridged_net_daemon:
	@echo "bridged_net_daemon is included in build all, no need extra make"

colinux: $(BUILD_ROOT)/colinux/common/version.h $(LOCAL_FILES)

driver: $(BUILD_PATH)/driver.o

INSTALL_FILES := $(INSTALL_FILES) \
	$(LOCAL_FILES)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS) $(BUILD_PATH)/driver.coff  $(BUILD_PATH)/driver.rc

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
