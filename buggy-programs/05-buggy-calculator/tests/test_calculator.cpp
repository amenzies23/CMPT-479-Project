#include <gtest/gtest.h>
#include "calculator.h"
#include <stdexcept>

TEST(CalculatorTest, Add) {
    Calculator calc(3, 4);
    EXPECT_EQ(calc.add(), 7);
}

TEST(CalculatorTest, Subtract) {
    Calculator calc(10, 5);
    EXPECT_EQ(calc.subtract(), 5);
}

TEST(CalculatorTest, Multiply) {
    Calculator calc(2, 5);
    EXPECT_EQ(calc.multiply(), 10);
}

TEST(CalculatorTest, Divide) {
    Calculator calc(8, 2);
    EXPECT_DOUBLE_EQ(calc.divide(), 4.0);
}

TEST(CalculatorTest, SecretCalculation) {
    Calculator calc(3, 4);
    // (3 * 4) + 3 - 4 = 12 + 3 - 4 = 11
    EXPECT_EQ(calc.secret_calculation(), 11);
}

TEST(CalculatorTest, SqrtFirst) {
    Calculator calc(9, 1);
    EXPECT_DOUBLE_EQ(calc.sqrt_first(), 3.0);
}

TEST(CalculatorTest, SqrtSecond) {
    Calculator calc(3, 4);
    EXPECT_DOUBLE_EQ(calc.sqrt_second(), 2.0);
}



TEST(CalculatorTest, AreaRectangle) {
    Calculator calc(3, 4);
    EXPECT_DOUBLE_EQ(calc.area_rectangle(), 12.0);
}