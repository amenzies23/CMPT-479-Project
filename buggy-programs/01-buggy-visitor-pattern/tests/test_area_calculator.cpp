#include <gtest/gtest.h>
#include "rectangle.h"
#include "circle.h"
#include "triangle.h"
#include "area_calculator.h"

TEST(AreaCalculatorTest, RectangleArea) {
    Rectangle r(2, 3);
    AreaCalculator calc;
    r.accept(calc);
    EXPECT_DOUBLE_EQ(calc.get_total_area(), 6.0);
}

TEST(AreaCalculatorTest, CircleArea) {
    Circle c(2);
    AreaCalculator calc;
    c.accept(calc);
    EXPECT_NEAR(calc.get_total_area(), 12.56, 0.01);
}

TEST(AreaCalculatorTest, TriangleArea) {
    Triangle t(4, 5);
    AreaCalculator calc;
    t.accept(calc);
    EXPECT_DOUBLE_EQ(calc.get_total_area(), 10.0);
}

TEST(AreaCalculatorTest, AllShapesVectorArea) {
    Rectangle r(2, 3);
    Circle c(2);
    Triangle t(4, 5);
    std::vector<Shape*> shapes = {&r, &c, &t};
    AreaCalculator calc;
    std::vector<double> expected = {6.0, 12.56, 10.0};
    for (size_t i = 0; i < shapes.size(); ++i) {
        shapes[i]->accept(calc);
        if (i == 1) {
            EXPECT_NEAR(calc.get_total_area(), expected[i], 0.01);
        } else {
            EXPECT_DOUBLE_EQ(calc.get_total_area(), expected[i]);
        }
    }
}
