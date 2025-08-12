#pragma once
#include "visitor.h"

class Circle : public Shape {
public:
    int radius;
    Circle(int radius);
    void accept(Visitor &v) override;
};
