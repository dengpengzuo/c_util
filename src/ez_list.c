#include <stddef.h>

#include "ez_list.h"

void init_list_head(list_head * list)
{
	list->next = list;
	list->prev = list;
}

int list_empty(list_head * head)
{
	return head->next == head;
}

static inline void __list_add(list_head * newNode, list_head * prev, list_head * next)
{
	next->prev = newNode;
	newNode->next = next;
	newNode->prev = prev;
	prev->next = newNode;
}

void list_add(list_head * newNode, list_head * head)
{
	__list_add(newNode, head, head->next);
}

static inline void __list_del(list_head * prev, list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

void list_del(list_head * entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}
