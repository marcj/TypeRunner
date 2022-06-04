#include <gtest/gtest.h>

#include "../core.h"
#include "../checker/compiler.h"
#include <vector>
#include <fmt/ranges.h>

using namespace ts;

TEST(mem, unint) {
    std::vector<unsigned char> bin;

    compiler::writeUint32(bin, 0, 1025);
    EXPECT_EQ(bin.size(), 4);

    compiler::writeUint32(bin, 4, 1026);
    EXPECT_EQ(bin.size(), 8);

    EXPECT_EQ(compiler::readUint32(bin, 0), 1025);
    EXPECT_EQ(compiler::readUint32(bin, 4), 1026);

    fmt::print("bin {}\n", bin);
}
