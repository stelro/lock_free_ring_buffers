#include <gtest/gtest.h>
#include <cmath>

TEST(MathTest, Addition) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_EQ(5 + 7, 12);
}

TEST(MathTest, Multiplication) {
    EXPECT_EQ(3 * 4, 12);
    EXPECT_EQ(7 * 8, 56);
}

TEST(MathTest, SquareRoot) {
    EXPECT_DOUBLE_EQ(std::sqrt(4.0), 2.0);
    EXPECT_DOUBLE_EQ(std::sqrt(9.0), 3.0);
}
