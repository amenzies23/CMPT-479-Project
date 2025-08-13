#include <iostream>
#include <vector>
#include "include/rectangle.h"
#include "include/circle.h"
#include "include/triangle.h"
#include "include/area_calculator.h"

int main() {
    Rectangle r(2, 3);
    Circle c(2);
    Triangle t(4, 5);
    AreaCalculator area_calc;

    std::vector<Shape*> shapes = {&r, &c, &t};

    for (auto shape : shapes) {
        shape->accept(area_calc);
        std::cout << "total area  = " << area_calc.get_total_area() << std::endl;
    }

    return 0;
}
