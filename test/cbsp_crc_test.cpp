#include <gtest/gtest.h>

#include "cbsp_crc.hpp"

#define ptest(fmt, ...) fprintf(stdout, "TESTING " fmt "\n", __VA_ARGS__)

TEST(CRCTest, RECAL)
{
    std::string str{"123456"};
    uint32_t crc = 0x0;
    crc = cbsp::crc32(str.c_str(), str.size(), crc);
    crc = cbsp::crc32("789", 3, crc);
    ASSERT_NE(crc, 0);
    ASSERT_EQ(crc, cbsp::crc32((str + "789").c_str(), str.size() + 3));
}