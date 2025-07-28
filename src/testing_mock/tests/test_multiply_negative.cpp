#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(-6, 7);
    if (calc.multiply() == -42) {
        std::cout << "test_multiply_negative passed!\n";
        return 0;
    } else {
        std::cout << "test_multiply_negative failed!\n";
        return 1;
    }
}
