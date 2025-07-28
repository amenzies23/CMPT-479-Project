#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(0, 5);
    if (calc.area_rectangle() == 0) {
        std::cout << "test_area_rectangle_zero passed!\n";
        return 0;
    } else {
        std::cout << "test_area_rectangle_zero failed!\n";
        return 1;
    }
}
