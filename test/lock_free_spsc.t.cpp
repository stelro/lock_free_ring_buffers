#include <gtest/gtest.h>
#include "lock_free_spsc.hpp"

TEST(LockFreeSPSC, QueueIsEmpty) {
    lock_free_spsc_queue<int, 4> q;
	ASSERT_TRUE(q.empty());
}

TEST(LockFreeSPSC, QueueIsEmpty_2) {
    lock_free_spsc_queue<int, 4> q;
	ASSERT_TRUE(q.empty());
	EXPECT_TRUE(q.try_push(1));
	ASSERT_FALSE(q.empty());
	EXPECT_TRUE(q.try_pop());
	ASSERT_TRUE(q.empty());
}

TEST(LockFreeSPSC, PushTest) {
    lock_free_spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	const auto front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 1);
}

TEST(LockFreeSPSC, PushAndPop) {
    lock_free_spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	auto front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 1);
	front = q.try_pop();
	ASSERT_FALSE(front.has_value());
	EXPECT_EQ(front, std::nullopt);
}

TEST(LockFreeSPSC, PushAndPopMultipleValues) {
    lock_free_spsc_queue<int, 4> q;
	EXPECT_TRUE(q.try_push(1));
	EXPECT_TRUE(q.try_push(2));
	EXPECT_TRUE(q.try_push(3));
	EXPECT_FALSE(q.try_push(4)); // Should Fail
	
	const auto front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 1);
	EXPECT_EQ(q.maybe_size(), 2);
}

TEST(LockFreeSPSC, PushAndPopMultipleValues_2) {
    lock_free_spsc_queue<int, 4> q;

	EXPECT_TRUE(q.try_push(1));
	auto front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 1);
	EXPECT_EQ(q.maybe_size(), 0);

	EXPECT_TRUE(q.try_push(2));
	front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 2);

	EXPECT_TRUE(q.try_push(3));
	front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 3);

	EXPECT_TRUE(q.try_push(4));
	front = q.try_pop();
	ASSERT_TRUE(front.has_value());
	EXPECT_EQ(front.value(), 4);

	EXPECT_EQ(q.maybe_size(), 0);
	EXPECT_TRUE(q.empty());
}


