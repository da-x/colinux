/*
 * This source code is a part of coLinux source package.
 *
 * Dan Aloni <da-x@colinux.org>, 2003 (c)
 *
 * The code is licensed under the GPL. See the COPYING file at
 * the root directory.
 */

#ifndef __COLINUX_LINUX_LIST_H__
#define __COLINUX_LINUX_LIST_H__

/*
 * Inspiried by the doubly linked list implementation from the Linux kernel.
 */

typedef struct co_list {
	struct co_list *next, *prev;
} co_list_t;

static inline void co_list_init(co_list_t *list)
{
	list->next = list;
	list->prev = list;
}

static inline int co_list_empty(co_list_t *list)
{
	return (list->next == list);
}

static inline void co_list_add(co_list_t *new_, co_list_t *prev, co_list_t *next)
{
	next->prev = new_;
	new_->next = next;
	new_->prev = prev;
	prev->next = new_;
}

static inline void co_list_add_head(co_list_t *new_, co_list_t *head)
{
	co_list_add(new_, head, head->next);
}

static inline void co_list_add_tail(co_list_t *new_, co_list_t *head)
{
	co_list_add(new_, head->prev, head);
}

static inline void co_list_unlink(co_list_t *prev, co_list_t *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void co_list_del(co_list_t *entry)
{
	co_list_unlink(entry->prev, entry->next);
}

/**
 * co_list_entry - get the struct for this entry
 * @ptr:	the &co_list_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the co_list_t within the struct.
 */
#define co_list_entry(ptr, type, member)				\
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define co_list_entry_assign(ptr, var, member)				\
	var = co_list_entry(ptr, typeof(*var), member)

/**
 * list_for_each - iterate over a list
 * @pos:	the &co_list_t to use as a loop counter.
 * @head:	the head for your list.
 */
#define co_list_each(pos, head) \
	(pos = (head)->next; pos != (head); pos = pos->next)

#define co_list_each_entry(pos, head, member)				\
	for (co_list_entry_assign((head)->next, pos, member);		\
	     &pos->member != (head);					\
	     co_list_entry_assign(pos->member.next, pos, member))	\
									\

#define co_list_each_entry_safe(pos, pos_next, head, member)		\
	for (co_list_entry_assign((head)->next, pos, member),		\
             co_list_entry_assign(pos->member.next, pos_next, member);	\
                                                                        \
             &pos->member != (head);                                    \
                                                                        \
             pos = pos_next,                                            \
             co_list_entry_assign(pos->member.next, pos_next, member))  \


#endif
