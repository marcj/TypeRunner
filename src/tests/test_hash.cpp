#include <gtest/gtest.h>

#include "../hash.h"

using namespace std;
using namespace tr::hash;

TEST(hash, hash) {
    string_view sv = "foo188";
    string s = "foo188";

//    EXPECT_EQ("fo"_hash, 1842121476277988984UL);
//    EXPECT_EQ(const_hash("fo"), 1842121476277988984UL);
//    EXPECT_EQ(runtime_hash("fo"), 1842121476277988984UL);

    EXPECT_EQ("foo188"_hash, 3578094862220341077UL);
    EXPECT_EQ(runtime_hash(s), 3578094862220341077UL);
    EXPECT_EQ(runtime_hash(sv), 3578094862220341077UL);

    EXPECT_EQ(const_hash("foo188"), 3578094862220341077UL);
    EXPECT_EQ(runtime_hash("foo188"), 3578094862220341077UL);
}
