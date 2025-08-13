#include "stack-queue.h"
#include <gtest/gtest.h>

TEST(StackTest, PushPop) {
    Stack s;
    EXPECT_TRUE(s.empty());
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 3);
    EXPECT_EQ(s.pop(), 3);
    EXPECT_EQ(s.pop(), 2);
    EXPECT_EQ(s.pop(), 1);
    EXPECT_TRUE(s.empty());
}

TEST(StackTest, PopEmptyThrows) {
    Stack s;
    EXPECT_THROW(s.pop(), std::out_of_range);
}

TEST(QueueTest, PushPop) {
    Queue q;
    EXPECT_TRUE(q.empty());
    q.push(10);
    q.push(20);
    q.push(30);
    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.pop(), 10);
    EXPECT_EQ(q.pop(), 20);
    EXPECT_EQ(q.pop(), 30);
    EXPECT_TRUE(q.empty());
}

TEST(QueueTest, PopEmptyThrows) {
    Queue q;
    EXPECT_THROW(q.pop(), std::out_of_range);
}
