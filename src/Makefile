#
# This source code is a part of coLinux source package.
# 
# Dan Aloni <da-x@gmx.net>, 2003 (c)
#
# We don't use a recursive make, because a recursive make is considered
# harmful (see google). Instead, we have an hierarchy of makefiles that
# Is read using one invocation of make, in each of the directories.
# It works by using the 'include' directive of make. The only problem
# with this directive is that it doesn't change the current directory
# and nested includes are still relative to the directory where 'make'
# was run and not to the directory of the included file.
#
# So each included makefile needs to know its path from the root. But,
# I didn't want to put these hardcoded pathnames inside every makefile,
# so what I did was using $(BUILD_PATH) as a stack to reflect the current
# path of the included makefile.
#
# Each included makefile adds its directory name to $(BUILD_PATH) and in
# its beginning and removes it in in the end. If someone runs make in the
# included makefile's directory, the makefile includes the root makefile
# while settings BUILD_ROOT to reflect the root's relative path from the
# current directory.
#

COLINUX_ARCH_DEFAULT=i386
COLINUX_HOST_OS_DEFAULT=winnt

ifeq ($(BUILD_ROOT),)
export COLINUX_HOST_OS_DEFAULT
export COLINUX_ARCH_DEFAULT

all:
	@./intro

BUILD_ROOT=.
endif

# Default architecture
ifeq ($(COLINUX_ARCH),)
COLINUX_ARCH=$(COLINUX_ARCH_DEFAULT)
endif

# Default host operating system
ifeq ($(COLINUX_HOST_OS),)
COLINUX_HOST_OS=$(COLINUX_HOST_OS_DEFAULT)
endif

#
# Update the symlink to the current architecture directory
#

ARCH_CURRENT=$(BUILD_ROOT)/colinux/arch/current
ifneq ($(shell readlink $(ARCH_CURRENT)),$(COLINUX_ARCH))
X:=$(shell rm -f $(ARCH_CURRENT))
X:=$(shell ln -s $(COLINUX_ARCH) $(ARCH_CURRENT))
endif

#
# Update the symlink to the current host OS directory
#

OS_CURRENT=$(BUILD_ROOT)/colinux/os/current
ifneq ($(shell readlink $(OS_CURRENT)),$(COLINUX_HOST_OS))
X:=$(shell rm -f $(OS_CURRENT))
X:=$(shell ln -s $(COLINUX_HOST_OS) $(OS_CURRENT))
endif

BUILD_SUBDIRS = $(shell echo $(addprefix $(BUILD_PATH)/,$(addsuffix /Makefile,$(1))))

# 
# Common tools definitions
#

ifeq ($(COLINUX_TARGET_KERNEL_PATH),)
$(error COLINUX_TARGET_KERNEL_PATH not set. Please set this environment variable to the pathname of a coLinux-enabled kernel source tree)
endif

CO_INCLUDES=-I$(BUILD_ROOT) -I$(COLINUX_TARGET_KERNEL_PATH)/include

#
# This define is used to resolve some conflicts between two
# operating systems' include definitions.
#

HOST_API_CFLAGS=-DCO_HOST_API

AR=$(CROSS_TARGET)ar
STRIP=$(CROSS_TARGET)strip

TARGET_FLAGS = TARGETFLAGS_$(subst /,_,$(BUILD_PATH))

COLINUX_ID_ADD = -DCOLINUX_FILE_ID=`python ../bin/file_id.py create $(FILES_LIST_FILE) $@`
TOOL_USER_CC= $(CC) $(COLINUX_ID_ADD) $(USER_CFLAGS) $(CO_INCLUDES) $(TARGETFLAGS_._$(subst /,_,$<)) -c $< -o $@ 
TOOL_USER_GPP= $(GPP) $(COLINUX_ID_ADD) $(USER_CFLAGS) $(CO_INCLUDES) $(TARGETFLAGS_._$(subst /,_,$<)) -c $< -o $@ 
TOOL_KERNEL_CC= $(CC) $(COLINUX_ID_ADD) $(KERNEL_CFLAGS) $(CO_INCLUDES) $(TARGETFLAGS_._$(subst /,_,$<)) -c $< -o $@ 
TOOL_AR=$(AR) cr $@ $^

TOOL_LD_RELOC=$(LD) -g -r -o $@ $^

.phony: colinux

BUILD_PATH=$(BUILD_ROOT)
FILES_LIST_FILE = ./colinux/common/files_list.txt

include $(BUILD_PATH)/colinux/Makefile

# Copy file into install directory and strip debug information form distribution executable
install:
	@$(foreach file,$(INSTALL_FILES), echo install $(file); $(STRIP) $(file) -o $(COLINUX_INSTALL_DIR)/$(notdir $(file)); )

$(shell python ../bin/file_id.py update $(FILES_LIST_FILE) $(filter %.o,$(CLEAN_FILES)))

CLEAN_FILES := $(CLEAN_FILES) \
	$(FILES_LIST_FILE)

clean:
	@$(foreach file,$(CLEAN_FILES), echo rm -f $(file) ; rm -f $(file) ; )
	rm -f $(ARCH_CURRENT) $(OS_CURRENT)
