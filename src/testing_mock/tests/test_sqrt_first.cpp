#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(9, 1);
    if (calc.sqrt_first() == 3.0) {
        std::cout << "test_sqrt_first passed!\n";
        return 0;
    } else {
        std::cout << "test_sqrt_first failed!\n";
        return 1;
    }
}
