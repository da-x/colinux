#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/reactor.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>

#include "unix.h"
#include "reactor.h"

co_rc_t co_os_reactor_select(co_reactor_t handle, int miliseconds)
{
	struct pollfd wait_list[handle->num_users];
	co_reactor_user_t users[handle->num_users];
	co_reactor_user_t user;
	int wait_time, ret;
	int index = 0, count = 0;
	co_rc_t rc = CO_RC(OK);

	co_list_each_entry(user, &handle->users, node) {
		wait_list[index].fd = user->os_data->fd;
		wait_list[index].events = POLLIN | POLLHUP | POLLERR;
		wait_list[index].revents = 0;
		users[index] = user;
		index += 1;
	}

	count = handle->num_users;

	wait_time = -1;
	if (miliseconds >= 0)
		wait_time = miliseconds;

	ret = poll(wait_list, count, wait_time);
	if (ret == 0)
		return CO_RC(OK);

	if (ret < 0)
		return CO_RC(ERROR);

	for (index=0; index < count; index++) {
		user = users[index];
		if (wait_list[index].revents & POLLIN) {
			rc = user->last_read_rc = user->os_data->read(user);
			if (!CO_OK(rc))
				break;
		}
		if (wait_list[index].revents & POLLERR) {
			return CO_RC(ERROR); /* TODO */
		}
		if (wait_list[index].revents & POLLHUP) {
			return CO_RC(BROKEN_PIPE);
		}
	}

	return rc;
}

static co_rc_t packet_read_complete(co_linux_reactor_packet_user_t handle)
{
	int size;

	size = read(handle->os_user.fd, handle->buffer, sizeof(handle->buffer));
	if (size <= 0)
		return CO_RC(ERROR);

	handle->user.received(&handle->user, handle->buffer, size);
	return CO_RC(OK);
}

static co_rc_t packet_send_whole(co_linux_reactor_packet_user_t handle,
				 unsigned char *buffer, unsigned long size)
{
	int written;

	written = write(handle->os_user.fd, buffer, size);
	if (written != size)
		return CO_RC(ERROR);
	return CO_RC(OK);
}

static co_rc_t packet_read(co_reactor_user_t user)
{
	return packet_read_complete((co_linux_reactor_packet_user_t)user);
}

static co_rc_t packet_send(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	return packet_send_whole((co_linux_reactor_packet_user_t)user, buffer, size);
}

static void packet_write(co_reactor_user_t user)
{
}

extern co_rc_t co_linux_reactor_packet_user_create(
	co_reactor_t reactor, int fd,
	co_reactor_user_receive_func_t receive,
	co_linux_reactor_packet_user_t *handle_out)
{
	co_linux_reactor_packet_user_t user;

	user = co_os_malloc(sizeof(*user));
	if (!user)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(user, 0, sizeof(*user));

	user->os_user.fd = fd;

	co_os_set_blocking(fd, PFALSE);

	user->user.os_data = &user->os_user;
	user->user.reactor = reactor;
	user->user.received = receive;
	user->os_user.read = packet_read;
	user->os_user.write = packet_write;
	user->user.send = packet_send;

	co_reactor_add(reactor, &user->user);

	*handle_out = user;

	return CO_RC(OK);
}

void co_linux_reactor_packet_user_destroy(co_linux_reactor_packet_user_t user)
{
	close(user->os_user.fd);
	co_reactor_remove(&user->user);
	co_os_free(user);
}
