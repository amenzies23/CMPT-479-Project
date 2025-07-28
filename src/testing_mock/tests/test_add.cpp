#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(3, 4);
    if (calc.add() == 7) {
        std::cout << "test_add passed!\n";
        return 0;
    } else {
        std::cout << "test_add failed!\n";
        return 1;
    }
}
