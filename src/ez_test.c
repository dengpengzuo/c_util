#include <stdio.h>

#include "ez_macro.h"
#include "ez_test.h"
/*
字背景颜色:                    字颜色:
40: 黑                           30: 黑
41: 红                           31: 红
42: 绿                           32: 绿
43: 黄                           33: 黄
44: 蓝                           34: 蓝
45: 紫                           35: 紫
46: 深绿                         36: 深绿
47：白色                         37：白色
\033[字背景颜色;字体颜色m字符串\033[0m
*/

#define COL_BEGIN(B,C)  "\033["#B";"#C"m"
#define COL_END         "\033[0m"

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
        p = EZ_CONTAINER_OF(ti, test_suit_t, next);
        fprintf(stdout, COL_BEGIN(40,34) "==== Test [%s] ..." COL_END "\n", p->name);
        p->func();
        fprintf(stdout, COL_BEGIN(40,34) "==== Test [%s] " COL_BEGIN(40, 32) "PASSED" COL_END "\n", p->name);
    }
}