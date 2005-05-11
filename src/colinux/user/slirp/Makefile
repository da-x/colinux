# Standard Makefile header
ifeq ($(BUILD_ROOT),)
.DEFAULT:
	make -C ../../.. $(MAKECMDGOALS)
else
BUILD_PATH := $(BUILD_PATH)/slirp
#-----------------------------------------------------------------

LOCAL_OBJS = \
	cksum.o if.o ip_icmp.o ip_input.o ip_output.o mbuf.o misc.o  \
	sbuf.o  tcp_input.o tcp_output.o compat.o bootp.o \
	tcp_timer.o udp.o socket.o tcp_subr.o  slirp.o

LOCAL_OBJS := $(addprefix $(BUILD_PATH)/,$(LOCAL_OBJS))

$(LOCAL_OBJS) : %.o : %.c
	$(TOOL_USER_CC)

$(BUILD_PATH)/slirp.a : $(LOCAL_OBJS)
	$(TOOL_AR)

CLEAN_FILES := $(CLEAN_FILES) \
	$(LOCAL_OBJS) $(BUILD_PATH)/slirp.a

#-----------------------------------------------------------------
BUILD_PATH := $(shell dirname $(BUILD_PATH))
endif
