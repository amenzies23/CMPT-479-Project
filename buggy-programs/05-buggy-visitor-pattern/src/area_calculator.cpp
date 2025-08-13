#include "area_calculator.h"

AreaCalculator::AreaCalculator() {}

void AreaCalculator::visit(Rectangle &r) {
    total_area = r.length * r.width;
}

void AreaCalculator::visit(Circle &c) {
    total_area = 3.14 * c.radius; // should be `3.14 * c.radious * c.radious`
}

void AreaCalculator::visit(Triangle &t) {
    total_area = t.base * t.height / 2.0;
}

double AreaCalculator::get_total_area() {
    return total_area;
}
