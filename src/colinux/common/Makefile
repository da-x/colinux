# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/common
#-----------------------------------------------------------------

LOCAL_OBJS = \
	queue.o \
	console.o \
	debug.o \
	errors.o \
	messages.o \
	version.o \
	libc.o \
	libc_strtol.o \
	snprintf.o \
	file_ids.o \
	unicode.o \


LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_KERNEL_CC)

$(BUILD_PATH)/file_ids.c : $(FILES_LIST_FILE) $(BUILD_PATH)/file_ids.py
	cat $(word 1, $^) | python $(word 2, $^) > $@

$(BUILD_PATH)/libc_strtol.c : $(BUILD_PATH)/libc.c
	ln -s $(notdir $^) $@

CLEAN_FILES := $(CLEAN_FILES) \
	$(BUILD_PATH)/file_ids.c \
	$(BUILD_PATH)/libc_strtol.c

$(TARGET_FLAGS)_libc.c = -DCO_LIBC__MISC
$(TARGET_FLAGS)_libc_strtol.c = -DCO_LIBC__STRTOL

$(BUILD_PATH)/version.h : $(BUILD_PATH)/version.py $(BUILD_PATH)/../VERSION
	python $(word 1,$^) $(word 2,$^) $@

$(BUILD_PATH)/version.o : $(BUILD_PATH)/version.h

$(BUILD_PATH)/version.o : $(BUILD_PATH)/version.h

$(BUILD_PATH)/common.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/common.a \
	$(BUILD_PATH)/version.h

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
