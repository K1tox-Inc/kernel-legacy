#ifndef LIST_H
#define LIST_H

#include <types.h>
#include <utils/kmacro.h>

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name)                                                                       \
	{                                                                                              \
		&(name), &(name)                                                                           \
	}

#define INIT_SENTINEL(ptr) (*(ptr) = (struct list_head){.next = ptr, .prev = ptr})

#define list_is_empty(head) ((head) == (head)->next)

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_next_entry(pos, member) list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member) (&pos->member == (head))

#define list_first_entry(ptr, type, member)                                                        \
	(!list_is_empty(ptr) ? list_entry((ptr)->next, type, member) : NULL)

#define list_for_each_entry(pos, head, member)                                                     \
	for (pos = list_first_entry(head, typeof(*pos), member);                                       \
	     pos != NULL && !list_entry_is_head(pos, head, member);                                    \
	     pos = list_next_entry(pos, member))

#define list_for_each_safe(pos, tmp, head)                                                         \
	for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

static inline void list_add_head(struct list_head *new_node, struct list_head *head)
{
	struct list_head *old_next = head->next;

	new_node->next = old_next;
	new_node->prev = head;
	old_next->prev = new_node;
	head->next     = new_node;
}

static inline bool list_node_is_linked(struct list_head *node)
{
	return node && node->next && node->prev && node->next != node;
}

static inline void list_insert(struct list_head *new, struct list_head *prev,
                               struct list_head *next)
{
	next->prev = new;
	new->next  = next;
	new->prev  = prev;
	prev->next = new;
}

static inline void list_add_tail(struct list_head *new_node, struct list_head *head)
{
	list_insert(new_node, head->prev, head);
}

static inline void pop_node(struct list_head *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next       = NULL;
	node->prev       = NULL;
}

#endif
