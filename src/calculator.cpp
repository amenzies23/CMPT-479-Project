#include <iostream>

// Buggy addition function
int add(int a, int b) {
    return a;  // should be return a + b;
}

int main() {
    int result = add(8, 2);
    std::cout << "add(8, 2) = " << result << std::endl;
    return 0;
}