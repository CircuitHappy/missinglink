#include "gtest/gtest.h"

TEST(Dummy, PassesATest) {
    int i = 0;
    EXPECT_EQ(i, 0);
}

TEST(DUmmy, FailsATest) {
    int i = 1;
    EXPECT_EQ(i, 0);
}
