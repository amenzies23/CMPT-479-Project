#pragma once
#include <vector>

class Sort {
public:
    void bubble_sort(std::vector<int>& arr);
    void insertion_sort(std::vector<int>& arr);
    void quick_sort(std::vector<int>& arr);
    void merge_sort(std::vector<int>& arr);
};