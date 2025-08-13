#include <iostream>
#include "include/linked_list.h"

int main() {
    LinkedList list;
    list.append(1);
    list.append(2);
    list.append(3);
    std::cout << "List contains 2? " << (list.contains(2) ? "yes" : "no") << std::endl;
    std::cout << "List size: " << list.size() << std::endl;
    list.remove(2);
    std::cout << "List contains 2 after removal? " << (list.contains(2) ? "yes" : "no") << std::endl;
    std::cout << "List size after removal: " << list.size() << std::endl;
    return 0;
}
