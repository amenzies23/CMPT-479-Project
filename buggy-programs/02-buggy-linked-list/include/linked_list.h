#pragma once

struct Node {
    int value;
    Node* next;
    Node(int v) : value(v), next(nullptr) {}
};

class LinkedList {
public:
    LinkedList();
    ~LinkedList();
    void append(int value);
    bool remove(int value);
    bool contains(int value) const;
    int size() const;
private:
    Node* head;
};
