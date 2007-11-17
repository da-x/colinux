#
# Makefile for the coLinux Linux kernel module.
#

MODULE_NAME = colinux

# Softlinks avoids problems with userland builds
obj-y += .module.arch_build/
obj-y += .module.common_build/
obj-y += .module.kernel_build/

obj-m := $(MODULE_NAME).o

include $(KBUILD_EXTMOD)/Makefile.$(MODULE_NAME)-objs

$(MODULE_NAME)-objs += \
	.module.arch_build/lib.a \
	.module.common_build/lib.a \
	.module.kernel_build/lib.a \

# Solve circular references between archives
EXTRA_LDFLAGS += --start-group
