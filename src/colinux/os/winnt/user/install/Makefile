# Standard Makefile header
ifeq ($(BUILD_ROOT),)
BUILD_ROOT=../../../../..
include $(BUILD_ROOT)/Makefile
else
BUILD_PATH := $(BUILD_PATH)/install
#-----------------------------------------------------------------

NSIS_INSTALL_PATH=c:\\Program\ Files\\NSIS

installer: $(BUILD_PATH)/coLinux.exe

$(BUILD_PATH)/coLinux.exe: $(BUILD_PATH)/colinux.nsi $(BUILD_PATH)/colinux_def.inc bridged_net_daemon colinux 
	wine $(NSIS_INSTALL_PATH)\\makensis.exe $<

$(BUILD_PATH)/colinux_def.inc : $(BUILD_PATH)/colinux_def.py $(BUILD_ROOT)/colinux/VERSION
	python $(word 1,$^) $(word 2,$^) $@
    
CLEAN_FILES := $(CLEAN_FILES) \
	$(BUILD_PATH)/coLinux.exe \
	$(BUILD_PATH)/colinux_def.inc \
	

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
