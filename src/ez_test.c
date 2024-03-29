#include "ez_test.h"
#include "ez_util.h"
#include "ez_macro.h"
#include <stdio.h>

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

#define COL_BEGIN(B, C) "\033[" #B ";" #C "m"
#define COL_END "\033[0m"

typedef struct test_suit_s {
    list_head_t tests;
} test_suit_t;

static test_suit_t default_suite;

void init_default_suite()
{
    init_list_head(&default_suite.tests);
}

void suite_add_test(test_t* test)
{
    list_add(&test->next, &default_suite.tests);
}

void run_default_suite()
{
    test_t* p;

    LIST_FOR_R(&default_suite.tests, ti)
    {
        /*
         * [ RUN      ] ArenaTest.Empty
         * /work/cxx-code/Leveldb/util/arena_test.cc:46: Failure
         *  Expected: (arena.MemoryUsage()) <= (bytes), actual: 4104 vs 1
         * [       OK ] ArenaTest.Empty (0 ms)
         */
        p = EZ_CONTAINER_OF(ti, test_t, next);
        fprintf(stdout, COL_BEGIN(40, 32) "[ RUN        ] " COL_BEGIN(40, 37) "%s" COL_END "\n", p->name);
        int64_t begin = mstime();
        p->func(p);
        int64_t cst = mstime() - begin;

        switch (p->result) {
        case R_FAIL_BREAK:
            fprintf(stdout, "%s\n", p->msg);
            fprintf(stdout, COL_BEGIN(40, 31) "[     FAILED ] " COL_BEGIN(40, 37) "%s (%ld ms)" COL_END "\n", p->name, cst);
            break;
        default:
            fprintf(stdout, COL_BEGIN(40, 32) "[         OK ] " COL_BEGIN(40, 37) "%s (%ld ms)" COL_END "\n", p->name, cst);
        }

        if (p->result == R_FAIL_BREAK) {
            break;
        }
    }
}