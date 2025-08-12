#include "../include/linked_list.h"

LinkedList::LinkedList() : head(nullptr) {}

LinkedList::~LinkedList() {
    while (head) {
        Node* tmp = head;
        head = head->next;
        delete tmp;
    }
}

void LinkedList::append(int value) {
    Node* new_node = new Node(value);
    if (!head) {
        head = new_node;
        return;
    }
    Node* curr = head;
    while (curr->next) curr = curr->next;
    curr->next = new_node;
}

bool LinkedList::remove(int value) {
    Node* curr = head;
    Node* prev = nullptr;
    while (curr) {
        if (curr->value != value) { // should be if (curr->value == value) {
            if (prev) {
                prev->next = curr->next;
            } else {
                head = curr->next;
            }
            delete curr;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

bool LinkedList::contains(int value) const {
    Node* curr = head;
    while (curr) {
        if (curr->value == value) return true;
        curr = curr->next;
    }
    return false;
}

int LinkedList::size() const {
    int count = 0;
    Node* curr = head;
    while (curr) {
        ++count;
        curr = curr->next;
    }
    return count;
}
