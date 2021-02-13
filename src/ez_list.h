#ifndef EZ_LIST_H
#define EZ_LIST_H

typedef struct list_head_s
{
    struct list_head_s *next, *prev;
} list_head_t;

void init_list_head(list_head_t *list);

void list_add(list_head_t *newNode, list_head_t *head);

void list_del(list_head_t *entry);

int list_is_empty(list_head_t *head);

void list_foreach(list_head_t *head, void (*func)(list_head_t *entry));

#define LIST_FOR(head, pos) \
    for (list_head_t * (pos) = (head)->next, *n = (pos)->next; (pos) != head; (pos) = n, n = (pos)->next)

#define LIST_FOR_R(head, pos) \
    for (list_head_t * (pos) = (head)->prev, *n = (pos)->prev; (pos) != head; (pos) = n, n = (pos)->prev)

typedef enum
{
    BREAK_EACH = 0,
    CONTINUE_EACH
} EACH_RESULT;

void list_foreach2(list_head_t *head, EACH_RESULT (*func)(list_head_t *entry));

#endif /* EZ_LIST_H */
