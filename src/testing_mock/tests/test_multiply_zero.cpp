#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(0, 7);
    if (calc.multiply() == 0) {
        std::cout << "test_multiply_zero passed!\n";
        return 0;
    } else {
        std::cout << "test_multiply_zero failed!\n";
        return 1;
    }
}
