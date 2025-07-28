#include "../include/calculator.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    Calculator calc(5, 0);
    try {
        calc.divide();
        std::cerr << "Expected exception for divide by zero not thrown!\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << "test_divide_by_zero passed!\n";
        return 0;
    }
}
