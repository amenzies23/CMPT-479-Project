#pragma once
#include <vector>
#include <cstddef>

class Stack {
    std::vector<int> data;
public:
    void push(int value);
    int pop();
    int top() const;
    bool empty() const;
    size_t size() const;
};

class Queue {
    std::vector<int> data;
public:
    void push(int value);
    int pop();
    int front() const;
    bool empty() const;
    size_t size() const;
};
