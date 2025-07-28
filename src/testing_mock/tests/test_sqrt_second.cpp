#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(1, 16);
    if (calc.sqrt_second() == 4.0) {
        std::cout << "test_sqrt_second passed!\n";
        return 0;
    } else {
        std::cout << "test_sqrt_second failed!\n";
        return 1;
    }
}
