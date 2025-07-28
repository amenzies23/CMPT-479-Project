#include "../include/calculator.h"
#include <cassert>
#include <iostream>

int main() {
    Calculator calc(0, 10);
    if (calc.add() == 10) {
        std::cout << "test_add_zero passed!\n";
        return 0;
    } else {
        std::cout << "test_add_zero failed!\n";
        return 1;
    }
}
