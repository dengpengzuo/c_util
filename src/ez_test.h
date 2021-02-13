
#ifndef EZ_CUTIL_EZ_TEST_H
#define EZ_CUTIL_EZ_TEST_H

#include "ez_list.h"

#define TCONCAT3(a, b, c) TCONCAT3I(a, b, c)
#define TCONCAT3I(a, b, c) a##b##c

#define CHAR_TEXT(a, b) CHAR_TEXT1(a, b)
#define CHAR_TEXT1(a, b) #a #b

typedef void (*TEST_FUNC)();

typedef struct test_s
{
    const char *name;
    TEST_FUNC   func;
    list_head_t next;
} test_t;

void init_default_suite();

void suite_add_test(test_t *test);

void run_default_suite();

#define TEST(B, N)                                                                                       \
    static void   TCONCAT3(B, N, _test)();                                                               \
    static test_t TCONCAT3(B, N, _test_def) = {.name = CHAR_TEXT(B, N), .func = &TCONCAT3(B, N, _test)}; \
    static void   TCONCAT3(B, N, _test)()

#define SUITE_ADD_TEST(base, name) suite_add_test(&TCONCAT3(base, name, _test_def))

#endif // EZ_CUTIL_EZ_TEST_H
