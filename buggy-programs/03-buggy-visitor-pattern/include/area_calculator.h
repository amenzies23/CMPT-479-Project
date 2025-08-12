#pragma once
#include "visitor.h"
#include "rectangle.h"
#include "circle.h"
#include "triangle.h"

class AreaCalculator : public Visitor {
public:
    AreaCalculator();
    void visit(Rectangle &r) override;
    void visit(Circle &c) override;
    void visit(Triangle &t) override;
    double get_total_area();
private:
    double total_area = 0;
};
