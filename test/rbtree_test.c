#include <ez_test.h>

TEST(base, world)
{
    ASSERT_GE(1, 0);
}

TEST(base, world2)
{
    ASSERT_EQ(2, 2);
}

int main(int argc, char** argv)
{
    init_default_suite();
    SUITE_ADD_TEST(base, world);
    SUITE_ADD_TEST(base, world2);
    run_default_suite();
    return 0;
}