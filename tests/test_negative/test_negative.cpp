#include <iostream>
#include "add.h"

int main() {
    Add add(-5, -3);
    int result = add.get_sum();
    if (result == -8) {
        std::cout << "test_negative_numbers: PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "test_negative_numbers: FAILED (expected -8, got " << result << ")" << std::endl;
        return 1;
    }
}
