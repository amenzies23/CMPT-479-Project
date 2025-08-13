#include "rectangle.h"

Rectangle::Rectangle(int length, int width) : length(length), width(width) {}

void Rectangle::accept(Visitor &v) {
    v.visit(*this);
}
