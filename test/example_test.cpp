#include <gtest/gtest.h>

TEST(ExampleTest, BasicAssertion) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(ExampleTest, StringComparison) {
    std::string hello = "Hello";
    std::string world = "World";
    EXPECT_NE(hello, world);
}
