#include <iostream>
#include "add.h"

int main() {
    Add add(10, -3);
    int result = add.get_sum();
    if (result == 7) {
        std::cout << "test_mixed_numbers: PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "test_mixed_numbers: FAILED (expected 7, got " << result << ")" << std::endl;
        return 1;
    }
}
