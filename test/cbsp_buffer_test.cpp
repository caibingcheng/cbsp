#include <gtest/gtest.h>

#include "cbsp_buffer.hpp"

#define ptest(fmt, ...) fprintf(stdout, "TESTING " fmt "\n", __VA_ARGS__)

TEST(BufferTest, REPLACE)
{
    cbsp::Buffer buffer;
    for (int i = 0; i < 100; i++)
    {
        // Create a buffer to busy and reset to free after disconstruct
        buffer = cbsp::Buffer(1024 * 10 * 10);
        // hold 2 buffer max
        // 1. create a new buffer
        // 2. var.buffer hold the buffer
        // 3. in next loop create anthor new buffer
        // 4. var.buffer reset the holded buffer and hold the new buffer
        // 5. in next loop, var.buffer reset the holded buffer and hold the valid buffer
        ASSERT_GE(cbsp::Buffer::validCount(), 0);
        ASSERT_LE(cbsp::Buffer::validCount(), 1);
    }
    ASSERT_EQ(buffer.size(), 1024 * 10 * 10);
    ASSERT_NE(buffer.get(), nullptr);
    ASSERT_EQ(cbsp::Buffer::validCount(), 1);
    ASSERT_EQ(cbsp::Buffer::inValidCount(), 1);
    ASSERT_EQ(cbsp::Buffer::count(), 2);
}

TEST(BufferTest, PUSH)
{
    cbsp::Buffer::clear();
    cbsp::Buffer buffer_list[100];
    for (int i = 0; i < 100; i++)
    {
        auto &bl = buffer_list[i];
        ASSERT_EQ(bl.get(), nullptr);
        ASSERT_EQ(bl.size(), 0);

        bl = cbsp::Buffer(1024 * 10 * 10);
        ASSERT_EQ(bl.size(), 1024 * 10 * 10);
        ASSERT_NE(bl.get(), nullptr);
    }
    ASSERT_EQ(cbsp::Buffer::validCount(), 0);
    ASSERT_EQ(cbsp::Buffer::inValidCount(), 100);
    ASSERT_EQ(cbsp::Buffer::count(), 100);
}

TEST(BufferTest, SELECT)
{
    cbsp::Buffer::clear();
    for (int i = 0; i < 100; i++)
    {
        // init buffers
        cbsp::Buffer(1024 * i + 1);
    }
    ASSERT_EQ(cbsp::Buffer::validCount(), 10);
    ASSERT_EQ(cbsp::Buffer::inValidCount(), 0);
    ASSERT_EQ(cbsp::Buffer::count(), 10);

    cbsp::Buffer buffer;
    for (int i = 0; i < 100; i++)
    {
        buffer = cbsp::Buffer(1024 * 10);
        // select the minimum greater size in free list
        ASSERT_EQ(buffer.size(), (i % 2 == 0) ? 1024 * 90 + 1 : 1024 * 91 + 1);
    }
    ASSERT_EQ(cbsp::Buffer::validCount(), 9);
    ASSERT_EQ(cbsp::Buffer::inValidCount(), 1);
    ASSERT_EQ(cbsp::Buffer::count(), 10);
}