#include <cassert>

#include "calculator.h"

int main() {
    Calculator calc(10, 5);
    assert(calc.add() == 15);
    assert(calc.subtract() == 5);
    assert(calc.multiply() == 50);
    assert(calc.divide() == 2.0);
    assert(calc.area_rectangle() == 50);
    return 0;
}