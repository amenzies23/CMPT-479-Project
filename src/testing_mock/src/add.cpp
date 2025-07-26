#include "add.h"

Add::Add(int a, int b) : first_operand(a), second_operand(b) {};

int Add::get_sum() {
    return first_operand - second_operand;
}

