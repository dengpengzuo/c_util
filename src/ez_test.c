#include <stdio.h>

#include "ez_test.h"

static list_head_t default_suite;

void init_default_suite() {
    init_list_head(&default_suite);
}

void suite_add_test(test_suit_t *test) {
    list_add(&test->next, &default_suite);
}

void run_default_suite() {
    test_suit_t *p;
    LIST_FOR_R(&default_suite, ti) {
        p = cast_to_test_suit(ti);
        fprintf(stdout, "==== Test %s \n", p->name);
        p->func();
        fprintf(stdout, "==== Test %s passed \n", p->name);
    }
}