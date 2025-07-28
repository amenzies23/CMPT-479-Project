#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(-10, -5);
    if (calc.subtract() == -5) {
        std::cout << "test_subtract_negative passed!\n";
        return 0;
    } else {
        std::cout << "test_subtract_negative failed!\n";
        return 1;
    }
}
