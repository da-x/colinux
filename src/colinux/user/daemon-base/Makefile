# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/daemon-base
#-----------------------------------------------------------------

# -DDEBUG_CO_RC

LOCAL_OBJS = \
	main.o \

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.cpp
	$(TOOL_USER_GPP)

$(BUILD_PATH)/daemon-base.o : $(LOCAL_OBJS)
	$(TOOL_LD_RELOC)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/user-console.o 

#-----------------------------------------------------------------
# include $(call BUILD_SUBDIRS, )
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
