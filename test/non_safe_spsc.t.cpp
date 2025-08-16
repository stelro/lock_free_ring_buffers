#include <gtest/gtest.h>
#include "non_safe_spsc.hpp"

TEST(NonSafeSPSC, QueueIsEmpty) {
    spsc_queue<int, 4> q;
	ASSERT_TRUE(q.empty());
}

TEST(NonSafeSPSC, QueueIsEmpty_2) {
    spsc_queue<int, 4> q;
	ASSERT_TRUE(q.empty());
	EXPECT_TRUE(q.try_push(1));
	ASSERT_FALSE(q.empty());
	EXPECT_TRUE(q.try_pop());
	ASSERT_TRUE(q.empty());
}

TEST(NonSafeSPSC, PushTest) {
    spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 1);
}

TEST(NonSafeSPSC, PushAndPop) {
    spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 1);
	EXPECT_TRUE(q.try_pop());
	ASSERT_FALSE(q.front().has_value());
	EXPECT_EQ(q.front(), std::nullopt);
}

TEST(NonSafeSPSC, PushAndPopMultipleValues) {
    spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	EXPECT_TRUE(q.try_push(2));
	EXPECT_TRUE(q.try_push(3));
	EXPECT_TRUE(q.try_push(4));
	EXPECT_FALSE(q.try_push(5)); // Should faile

	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 1);
	EXPECT_EQ(q.size(), 4);
}

TEST(NonSafeSPSC, PushAndPopMultipleValues_2) {
    spsc_queue<int, 4> q;

	EXPECT_TRUE(q.try_push(1));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 1);
	EXPECT_EQ(q.size(), 1);
	EXPECT_TRUE(q.try_pop());

	EXPECT_TRUE(q.try_push(2));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 2);
	EXPECT_TRUE(q.try_pop());

	EXPECT_TRUE(q.try_push(3));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 3);
	EXPECT_TRUE(q.try_pop());

	EXPECT_TRUE(q.try_push(4));
	ASSERT_TRUE(q.front().has_value());
	EXPECT_EQ(q.front().value(), 4);
	EXPECT_TRUE(q.try_pop());

	EXPECT_EQ(q.size(), 0);
	EXPECT_TRUE(q.empty());
}


