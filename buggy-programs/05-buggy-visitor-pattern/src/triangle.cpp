#include "triangle.h"

Triangle::Triangle(int base, int height) : base(base), height(height) {}

void Triangle::accept(Visitor &v) {
    v.visit(*this);
}
