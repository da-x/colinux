#include <colinux/os/alloc.h>
#include <colinux/common/libc.h>
#include <colinux/os/user/reactor.h>

co_rc_t co_reactor_create(co_reactor_t *out_handle)
{
	co_reactor_t reactor;

	reactor = co_os_malloc(sizeof(*reactor));
	if (!reactor)
		return CO_RC(OUT_OF_MEMORY);

	co_memset(reactor, 0, sizeof(*reactor));

	co_list_init(&reactor->users);

	*out_handle = reactor;

	return CO_RC(OK);
}

co_rc_t co_reactor_select(co_reactor_t reactor, int miliseconds)
{
	return co_os_reactor_select(reactor, miliseconds);
}

void co_reactor_add(co_reactor_t reactor, co_reactor_user_t user)
{
	co_list_add_head(&user->node, &reactor->users);
	reactor->num_users += 1;
	user->reactor = reactor;
}

void co_reactor_remove(co_reactor_user_t user)
{
	user->reactor->num_users -= 1;
	co_list_del(&user->node);
}

void co_reactor_destroy(co_reactor_t reactor)
{
	co_os_free(reactor);
}

