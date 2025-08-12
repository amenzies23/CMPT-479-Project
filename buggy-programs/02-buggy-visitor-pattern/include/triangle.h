#pragma once
#include "visitor.h"

class Triangle : public Shape {
public:
    int base;
    int height;
    Triangle(int base, int height);
    void accept(Visitor &v) override;
};
