#include "circle.h"

Circle::Circle(int radius) : radius(radius) {}

void Circle::accept(Visitor &v) {
    v.visit(*this);
}
