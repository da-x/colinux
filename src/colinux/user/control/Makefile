# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/control
#-----------------------------------------------------------------

# -DDEBUG_CO_RC

LOCAL_OBJS = \
	main.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_USER_GCC)

$(BUILD_PATH)/control.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS)

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
