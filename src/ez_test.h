
#ifndef EZ_CUTIL_EZ_TEST_H
#define EZ_CUTIL_EZ_TEST_H

#include "ez_list.h"

#define TCONCAT3(a, b, c) TCONCAT3I(a, b, c)
#define TCONCAT3I(a, b, c) a##b##c

#define CHAR_TEXT3(a, b, c) #a b #c

#define STR2(a) STR1(a)
#define STR1(a) #a

typedef int TEST_RESULT;

#define R_CONTINUE    0
#define R_FAIL_BREAK  1

typedef struct test_s test_t;

typedef void (*TEST_FUNC)(test_t*);

struct test_s {
    const char* name;
    TEST_FUNC   func;
    list_head_t next;
    char       *msg;
    TEST_RESULT result;
};

void init_default_suite();

void suite_add_test(test_t* test);

void run_default_suite();

#define TEST(B, N)                                                                                                                    \
    static void TCONCAT3(B, N, _test)(test_t * __test_result);                                                                       \
    static struct test_s TCONCAT3(B, N, _test_def) = { .name = CHAR_TEXT3(B, ".", N), .func = &TCONCAT3(B, N, _test), .result = R_CONTINUE };\
    static void TCONCAT3(B, N, _test)(test_t* __test_result)

#define ASSERT_EQ(var1, var2) \
    ASSERT_PRED_(CmpEQ, var1, var2, R_FAIL_BREAK)

#define ASSERT_GE(var1, var2) \
    ASSERT_PRED_(CmpGE, var1, var2, R_FAIL_BREAK)

#define ASSERT_PRED_(pred_func, v1, v2, on_failure) \
    pred_func(v1, v2, on_failure)

#define ASSERT_FILE_LINE_3(a, b, c, d) \
    __FILE__ ":" STR2( __LINE__) d #a b #c

#define CmpGE(lv, rv, tr) \
    if (!((lv) > (rv))) { \
        __test_result->msg = ASSERT_FILE_LINE_3(lv, ">", rv, " failed: ") ; \
        __test_result->result = tr; \
    }

#define CmpEQ(lv, rv, tr) \
    if (!((lv) == (rv))) { \
        __test_result->msg = ASSERT_FILE_LINE_3(lv, "==", rv, " failed: ") ; \
        __test_result->result = tr; \
    }


#define SUITE_ADD_TEST(base, name) suite_add_test(&TCONCAT3(base, name, _test_def))

#endif // EZ_CUTIL_EZ_TEST_H
