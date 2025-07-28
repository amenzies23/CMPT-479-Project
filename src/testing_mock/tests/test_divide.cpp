#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(8, 2);
    if (calc.divide() == 4.0) {
        std::cout << "test_divide passed!\n";
        return 0;
    } else {
        std::cout << "test_divide failed!\n";
        return 1;
    }
}
