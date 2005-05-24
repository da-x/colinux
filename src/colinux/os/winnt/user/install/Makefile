# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/install
#-----------------------------------------------------------------

NSIS_INSTALL_PATH=c:\\Program\ Files\\NSIS

installer: $(BUILD_PATH)/coLinux.exe

$(BUILD_PATH)/coLinux.exe: \
	$(BUILD_PATH)/colinux.nsi \
	$(BUILD_PATH)/colinux_def.inc \
	$(COLINUX_INSTALL_DIR)/README.txt \
	$(COLINUX_INSTALL_DIR)/news.txt \
	$(COLINUX_INSTALL_DIR)/cofs.txt \
	$(COLINUX_INSTALL_DIR)/colinux-daemon.txt \
	$(COLINUX_INSTALL_DIR)/default.colinux.xml \
	$(COLINUX_INSTALL_DIR)/colinux-daemon.exe \
	$(COLINUX_INSTALL_DIR)/linux.sys \
	colinux
	@rm -f $(shell dirname $@)/distdir
	ln -s $(COLINUX_INSTALL_DIR) $(shell dirname $@)/distdir
	wine $(NSIS_INSTALL_PATH)\\makensis.exe $<

# Convert files into win-lite CR+LF and store in distdir
$(COLINUX_INSTALL_DIR)/README.txt: $(BUILD_ROOT)/../RUNNING
	unix2dos -n $^ $@

$(COLINUX_INSTALL_DIR)/news.txt: $(BUILD_ROOT)/../NEWS
	unix2dos -n $^ $@

$(COLINUX_INSTALL_DIR)/cofs.txt: $(BUILD_ROOT)/../doc/cofs
	unix2dos -n $^ $@

$(COLINUX_INSTALL_DIR)/colinux-daemon.txt: $(BUILD_ROOT)/../doc/colinux-daemon
	unix2dos -n $^ $@

$(COLINUX_INSTALL_DIR)/default.colinux.xml: $(BUILD_ROOT)/../conf/default.colinux.xml
	unix2dos -n $^ $@

# Grep kernel version, coLinux version, distdir
$(BUILD_PATH)/colinux_def.inc: \
	$(BUILD_PATH)/colinux_def.sh \
	$(BUILD_ROOT)/colinux/VERSION \
	$(COLINUX_INSTALL_DIR)/vmlinux
	. $^ > $@
    
CLEAN_FILES := $(CLEAN_FILES) \
	$(BUILD_PATH)/coLinux.exe \
	$(BUILD_PATH)/colinux_def.inc \
	$(BUILD_PATH)/distdir \
	

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
