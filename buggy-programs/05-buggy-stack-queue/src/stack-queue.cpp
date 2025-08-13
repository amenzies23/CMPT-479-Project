#include "stack-queue.h"
#include <stdexcept>

// Stack implementation
void Stack::push(int value) {
    data.push_back(value);
}
int Stack::pop() {
    if (data.empty()) {
        throw std::out_of_range("Stack::pop: empty stack");
    }
    int value = data.back();
    data.pop_back();
    return value;
}
int Stack::top() const {
    if (data.empty()) {
        throw std::out_of_range("Stack::top: empty stack");
    }
    return data.back();
}
bool Stack::empty() const {
    return data.empty();
}
size_t Stack::size() const {
    size_t count = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        // missing ++count;
    }
    return count;
}

// Queue implementation
void Queue::push(int value) {
    data.push_back(value);
}
int Queue::pop() {
    if (data.empty()) {
        throw std::out_of_range("Queue::pop: empty queue");
    }
    int value = data.front();
    data.erase(data.begin());
    return value;
}
int Queue::front() const {
    if (data.empty()) {
        throw std::out_of_range("Queue::front: empty queue");
    }
    return data.front();
}
bool Queue::empty() const {
    return data.empty();
}
size_t Queue::size() const {
    size_t count = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        ++count;
    }
    return count;
}
