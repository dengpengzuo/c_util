#ifndef EZ_LIST_H
#define EZ_LIST_H

typedef struct list_head_t {
    struct list_head_t *next, *prev;
} list_head;

void init_list_head(list_head *list);

void list_add(list_head *newNode, list_head *head);

void list_del(list_head *entry);

int list_is_empty(list_head *head);

void list_foreach(list_head *head, void (*func)(list_head *entry));

#define LIST_FOR(head) \
    for (list_head * pos = (head)->next, *n = pos->next ; pos != head; pos = n, n = pos->next)

typedef enum {
    BREAK_EACH = 0,
    CONTINUE_EACH
} EACH_RESULT;

void list_foreach2(list_head *head, EACH_RESULT (*func)(list_head *entry));

#endif /* EZ_LIST_H */
