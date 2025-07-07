#include <iostream>
#include "add.h"

int main() {
    Add add(0, 0);
    int result = add.get_sum();
    if (result == 0) {
        std::cout << "test_zero: PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "test_with_zero: FAILED (expected 0, got " << result << ")" << std::endl;
        return 1;
    }
}
