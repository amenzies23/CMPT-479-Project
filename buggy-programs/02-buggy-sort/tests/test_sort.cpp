#include <gtest/gtest.h>
#include "../include/sort.h"
#include <vector>

TEST(SortTest, BubbleSort) {
    Sort s;
    std::vector<int> arr = {5, 2, 9, 1, 5, 6};
    s.bubble_sort(arr);
    EXPECT_EQ(arr, (std::vector<int>{1, 2, 5, 5, 6, 9}));
}

TEST(SortTest, InsertionSort) {
    Sort s;
    std::vector<int> arr = {3, 7, 4, 9, 5, 2};
    s.insertion_sort(arr);
    EXPECT_EQ(arr, (std::vector<int>{2, 3, 4, 5, 7, 9}));
}

TEST(SortTest, QuickSort) {
    Sort s;
    std::vector<int> arr = {8, 4, 2, 9, 5, 7};
    s.quick_sort(arr);
    EXPECT_EQ(arr, (std::vector<int>{2, 4, 5, 7, 8, 9}));
}

TEST(SortTest, MergeSort) {
    Sort s;
    std::vector<int> arr = {10, 7, 8, 9, 1, 5};
    s.merge_sort(arr);
    EXPECT_EQ(arr, (std::vector<int>{1, 5, 7, 8, 9, 10}));
}