#
# Makefile for the Linux filesystems.
#
# 14 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
# 

ifneq ($(COLINUX_HOST_KERNEL_DIR),)
KERNEL_DIR=$(COLINUX_HOST_KERNEL_DIR)
KERNEL_BUILD=$(COLINUX_HOST_KERNEL_DIR)
else
KERNEL_DIR=/lib/modules/$(shell uname -r)
KERNEL_BUILD=$(KERNEL_DIR)/build
endif

MODULE_NAME = colinux

$(MODULE_NAME)-objs := \
	alloc.o \
	main.o \
	debug.o \
	timer.o \
	misc.o \
        pages.o \
	time.o \
	manager.o \
	file.o \
	lib.o \
	prelinked_driver.o \

MODULE_OBJECT := $(MODULE_NAME).ko
MODULE_ROOT := $(KERNEL_DIR)/kernel/drivers
TOP_DIR ?= $(KERNEL_BUILD)

EXTRA_CFLAGS += -DCO_KERNEL -DCO_HOST_API -D__KERNEL__ -DMODULE -I$(COLINUX_PWD) -I$(COLINUX_PWD)/../../linux/include

obj ?= .
src ?= .

obj-m = colinux.o

all: module

module:
	@make -C $(KERNEL_BUILD) SUBDIRS=$(PWD)

clean:
	rm -f *.o *.ko *.mod.c .test.* .tmp.*

load: 
	insmod test.ko

unload:
	rmmod test

$(obj)/prelinked_driver.o: 
	cp $< $@
