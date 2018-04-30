#include <stddef.h>

#include "ez_list.h"

void init_list_head(list_head_t * list)
{
	list->next = list;
	list->prev = list;
}

int list_is_empty(list_head_t *head)
{
	return head->next == head;
}

static inline void __list_add(list_head_t * newNode, list_head_t * prev, list_head_t * next)
{
	next->prev = newNode;
	newNode->next = next;
	newNode->prev = prev;
	prev->next = newNode;
}

void list_add(list_head_t * newNode, list_head_t * head)
{
	__list_add(newNode, head, head->next);
}

static inline void __list_del(list_head_t * prev, list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}

void list_del(list_head_t * entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

void list_foreach(list_head_t *head, void (*func)(list_head_t *entry))
{
	list_head_t *i, *t;
	for(i = head->next; i != head;) {
		t = i;
		i = i->next;
		func(t);
	}
}

void list_foreach2(list_head_t *head, EACH_RESULT (*func)(list_head_t *entry))
{
	list_head_t *i, *t;
	for(i = head->next; i != head;) {
		t = i;
		i = i->next;
		EACH_RESULT r = func(t);

		if (r == BREAK_EACH)
			break;
		else if( r == CONTINUE_EACH)
			continue;
	}
}
