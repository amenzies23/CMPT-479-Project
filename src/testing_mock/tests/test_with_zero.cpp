#include <iostream>
#include "add.h"

int main() {
    Add add(5, 0);
    int result = add.get_sum();
    if (result == 5) {
        std::cout << "test_with_zero: PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "test_with_zero: FAILED (expected 5, got " << result << ")" << std::endl;
        return 1;
    }
}
