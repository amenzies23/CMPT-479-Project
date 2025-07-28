#include "../include/calculator.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    Calculator calc(0, 0);
    try {
        calc.divide();
        std::cerr << "Expected exception for 0/0 not thrown!\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << "test_divide_zero passed!\n";
        return 0;
    }
}
