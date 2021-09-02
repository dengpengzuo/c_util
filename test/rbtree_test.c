#include <ez_test.h>
#include <ez_bytebuf.h>

TEST(test, bg_rw)
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

    free_bytebuf(buf);
}

TEST(test, le_rw)
{
    bytebuf_t* buf = new_bytebuf(64);
    bytebuf_write_int16_le(buf, 0x2f3f);
    bytebuf_write_int32_le(buf, 0x4f5f6f7f);
    bytebuf_write_int64_le(buf, 0x8f9f00008f9f0000);

    for (int i = 0; i < 10; ++i) {
        int16_t a = 0;
        bytebuf_read_int16_le(buf, &a);
        ASSERT_EQ(a, 0x2f3f);

        int32_t c = 0;
        bytebuf_read_int32_le(buf, &c);
        ASSERT_EQ(c, 0x4f5f6f7f);

        int64_t d = 0;
        bytebuf_read_int64_le(buf, &d);
        ASSERT_EQ(d, 0x8f9f00008f9f0000);

        bytebuf_reset_reader_index(buf);
        bytebuf_resize(buf, buf->cap * 2);
    }

    bytebuf_reset_writer_index(buf);
    while (bytebuf_is_writeable(buf)) {
        bytebuf_write_int8(buf, 'a');
    }

    free_bytebuf(buf);
}

int main(int argc, char** argv)
{
    init_default_suite();
    SUITE_ADD_TEST(test, bg_rw);
    SUITE_ADD_TEST(test, le_rw);
    run_default_suite();
    return 0;
}