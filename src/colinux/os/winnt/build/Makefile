# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../..

all: parts

include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/build
#-----------------------------------------------------------------

LOCAL_TARGETS = $(BUILD_PATH)/driver.o

$(BUILD_PATH)/driver.o : \
	$(BUILD_ROOT)/colinux/common/common.o \
	$(BUILD_ROOT)/colinux/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/os/current/kernel/kernel.o \
	$(BUILD_ROOT)/colinux/arch/current/arch.o \

	$(TOOL_LD_RELOC)

DRIVER_TARGET := $(BUILD_PATH)/linux.sys 

WINDIR=$(WINUSER)@$(WINBOX):$(WINREMOTEDIR)
WINBOX_SCP=scp -i ~/.ssh/locallan 
WINBOX_SSH=ssh -X -l $(WINUSER) -i ~/.ssh/locallan $(WINBOX)
WINLINK=$(WINBOX_SSH) /cygdrive/c/NTDDK/bin/link.exe

WINLINK_FLAGS= \
    /machine:ix86 \
    /STACK:262144,4096 \
    /MERGE:_PAGE=PAGE \
    /MERGE:_TEXT=.text \
    /SECTION:INIT,d \
    /OPT:REF \
    /OPT:ICF \
    /IGNORE:4001,4037,4039,4044,4065,4070,4078,4087,4089,4198 \
    /INCREMENTAL:NO \
    /FULLBUILD \
    /FORCE:MULTIPLE \
    /NOCOMMENT \
    /release \
    /NODEFAULTLIB \
    /debug:FULL \
    /debugtype:cv \
    /version:5.00 \
    /osversion:5.00 \
    /optidata \
    /driver \
    /align:0x1000 \
    /filealign:0x1000 \
    /subsystem:native,5.00 \
    /base:0x10000 \
    /entry:DriverEntry@8 \

WINLINK_LIBS= \
    C:\\\\NTDDK\\\\libfre\\\\i386\\\\ntoskrnl.lib \
    C:\\\\NTDDK\\\\libfre\\\\i386\\\\hal.lib \
    C:\\\\NTDDK\\\\libfre\\\\i386\\\\wmilib.lib \

$(DRIVER_TARGET): $(BUILD_PATH)/driver.o
	$(WINBOX_SCP) $^ $(WINDIR)
	$(WINLINK) /out:$(notdir $@) $(WINLINK_FLAGS) $(notdir $^) $(WINLINK_LIBS)
	$(WINBOX_SCP) $(WINDIR)$(notdir $@) $@

LOCAL_TARGET += $(DRIVER_TARGET)
LOCAL_FILES = \
	$(BUILD_ROOT)/../../linux/vmlinux \
	$(BUILD_ROOT)/colinux/os/current/user/daemon/colinux-daemon.exe \
	$(BUILD_ROOT)/colinux/os/current/user/console/colinux-console.exe \
	$(DRIVER_TARGET) \

LOCAL_TARGETS := $(LOCAL_TARGET) $(BUILD_PATH)/driver.o

colinux: $(LOCAL_FILES) 

parts: $(LOCAL_FILES)

upload: $(LOCAL_FILES)
	$(WINBOX_SCP) $^ $(WINDIR)

upload_console: $(BUILD_ROOT)/colinux/os/current/user/console/colinux-console.exe
	$(WINBOX_SCP) $^ $(WINDIR)

driver: $(BUILD_PATH)/driver.o
	

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_TARGETS)

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
