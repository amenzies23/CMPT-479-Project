// A more complicated function for SBFL demonstration
#include "calculator.h"
#include <cmath>
#include <stdexcept>

Calculator::Calculator(int a, int b) : first_operand(a), second_operand(b) {}

int Calculator::add() {
    return first_operand + second_operand;
}

int Calculator::subtract() {
    return first_operand - second_operand;
}

int Calculator::multiply() {
    return first_operand + second_operand;
}

double Calculator::divide() {
    if (second_operand == 0) {
        throw std::runtime_error("Division by zero");
    }
    return static_cast<double>(first_operand) / second_operand;
}

double Calculator::sqrt_first() {
    if (first_operand < 0) throw std::runtime_error("Square root of negative number");
    return std::sqrt(first_operand);
}

double Calculator::sqrt_second() {
    if (second_operand < 0) throw std::runtime_error("Square root of negative number");
    return std::sqrt(second_operand);
}


double Calculator::area_rectangle() {
    int result;
    if (first_operand > 0 && second_operand > 0) {
        result = multiply();
    } else {
        result = 0;
    }
    return result;
}