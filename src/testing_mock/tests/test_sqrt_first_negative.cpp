#include "../include/calculator.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    Calculator calc(-9, 1);
    try {
        calc.sqrt_first();
        std::cerr << "Expected exception for sqrt of negative not thrown!\n";
        return 1;
    } catch (const std::runtime_error& e) {
        std::cout << "test_sqrt_first_negative passed!\n";
        return 0;
    }
}
