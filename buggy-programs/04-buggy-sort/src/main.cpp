#include <iostream>
#include <vector>
#include "sort.h"

void print_vec(const std::vector<int>& arr) {
    for (int v : arr) std::cout << v << " ";
    std::cout << std::endl;
}

int main() {
    Sort s;
    std::vector<int> arr1 = {5, 2, 9, 1, 5, 6};
    std::vector<int> arr2 = arr1;
    std::vector<int> arr3 = arr1;
    std::vector<int> arr4 = arr1;

    s.bubble_sort(arr1);
    std::cout << "Bubble sort: ";
    print_vec(arr1);

    s.insertion_sort(arr2);
    std::cout << "Insertion sort: ";
    print_vec(arr2);

    s.quick_sort(arr3);
    std::cout << "Quick sort: ";
    print_vec(arr3);

    s.merge_sort(arr4);
    std::cout << "Merge sort: ";
    print_vec(arr4);

    return 0;
}