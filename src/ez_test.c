#include <stdio.h>

#include "ez_test.h"

static list_head_t test_root;

void init_default_suite() {
    init_list_head(&test_root);
}

void suite_add_test(test_suit_t *test) {
    init_list_head(&test->next);
    list_add(&test_root, &test->next);
}

void run_default_suite() {
    test_suit_t *p;
    LIST_FOR(&test_root, ti) {
        p = cast_to_test_suit(ti);
        fprintf(stdout, "==== Test %s \n", p->name);
        p->func();
        fprintf(stdout, "==== Test %s passed \n", p->name);
    }
}