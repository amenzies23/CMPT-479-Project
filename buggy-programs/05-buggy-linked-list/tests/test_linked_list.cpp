#include <gtest/gtest.h>
#include "../include/linked_list.h"

TEST(LinkedListTest, AppendAndContains) {
    LinkedList list;
    list.append(1);
    list.append(2);
    list.append(3);
    EXPECT_TRUE(list.contains(1));
    EXPECT_TRUE(list.contains(2));
    EXPECT_TRUE(list.contains(3));
    EXPECT_FALSE(list.contains(4));
}

TEST(LinkedListTest, Size) {
    LinkedList list;
    EXPECT_EQ(list.size(), 0);
    list.append(1);
    EXPECT_EQ(list.size(), 1);
    list.append(2);
    EXPECT_EQ(list.size(), 2);
}

TEST(LinkedListTest, Remove) {
    LinkedList list;
    list.append(1);
    list.append(2);
    list.append(3);
    EXPECT_TRUE(list.remove(2));
    EXPECT_FALSE(list.contains(2));
    EXPECT_EQ(list.size(), 2);
    EXPECT_TRUE(list.remove(1));
    EXPECT_FALSE(list.contains(1));
    EXPECT_EQ(list.size(), 1);
}
