#
# Makefile for the coLinux Linux kernel module.
#

MODULE_NAME = colinux

# Softlinks avoids problems with userland builds
obj-y += .module.arch_build/
obj-y += .module.common_build/
obj-y += .module.kernel_build/

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs += \
	alloc.o \
	main.o \
	debug.o \
	timer.o \
	misc.o \
	pages.o \
	time.o \
	manager.o \
	file.o \
	mutex.o \
	user.o \
	wait.o \
	fs.o \
	core.o \
	.module.arch_build/lib.a \
	.module.common_build/lib.a \
	.module.kernel_build/lib.a \

ifeq ($(COLINUX_TARGET_KERNEL_BUILD),)
COLINUX_TARGET_KERNEL_BUILD := $(COLINUX_TARGET_KERNEL_PATH)
endif

include $(KBUILD_EXTMOD)/Makefile.include

ifneq ($(COLINUX_NDEBUG),Y)
EXTRA_CFLAGS += -DCOLINUX_DEBUG 
endif

EXTRA_CFLAGS += -DCO_HOST_API

# Deal with recursive depens of libraries
LDFLAGS_MODULE += --start-group
