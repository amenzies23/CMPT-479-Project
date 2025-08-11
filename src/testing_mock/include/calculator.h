#ifndef CALCULATOR_H
#define CALCULATOR_H


class Calculator {
private:
    int first_operand;
    int second_operand;
public:
    Calculator(int a, int b);
    int add();
    int subtract();
    int multiply();
    int secretCalculation();
    double divide();

    double sqrt_first();
    double sqrt_second();

    double area_rectangle();
};

#endif