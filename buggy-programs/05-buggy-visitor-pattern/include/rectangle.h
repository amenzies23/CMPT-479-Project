#pragma once
#include "visitor.h"

class Rectangle : public Shape {
public:
    int length;
    int width;
    Rectangle(int length, int width);
    void accept(Visitor &v) override;
};
