#include <ez_test.h>
#include <ez_bytebuf.h>

TEST(base, world)
{
    bytebuf_t* buf = new_bytebuf(64);
    bytebuf_write_int8(buf, 0x1f);
    bytebuf_write_int16(buf, 0x2f3f);
    bytebuf_write_int32(buf, 0x4f5f6f7f);
    bytebuf_write_int64(buf, 0x8f9f00008f9f0000);
    int8_t a = 0;
    bytebuf_read_int8(buf, &a);
    ASSERT_EQ(a, 0x1f);

    int16_t b = 0;
    bytebuf_read_int16(buf, &b);
    ASSERT_EQ(b, 0x2f3f);

    int32_t c = 0;
    bytebuf_read_int32(buf, &c);
    ASSERT_EQ(c, 0x4f5f6f7f);

    int64_t d = 0;
    bytebuf_read_int64(buf, &d);
    ASSERT_EQ(d, 0x8f9f00008f9f0000);
}

TEST(base, world2)
{
    ASSERT_GE(3, 2);
}

int main(int argc, char** argv)
{
    init_default_suite();
    SUITE_ADD_TEST(base, world);
    SUITE_ADD_TEST(base, world2);
    run_default_suite();
    return 0;
}