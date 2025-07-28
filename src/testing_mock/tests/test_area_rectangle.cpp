#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(5, 6);
    if (calc.area_rectangle() == 30) {
        std::cout << "test_area_rectangle passed!\n";
        return 0;
    } else {
        std::cout << "test_area_rectangle failed!\n";
        return 1;
    }
}
