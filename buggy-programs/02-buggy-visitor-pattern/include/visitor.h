#pragma once
#include <iostream>

class Rectangle;
class Circle;
class Triangle;

class Visitor {
public:
    virtual void visit(Rectangle &r) = 0;
    virtual void visit(Circle &c) = 0;
    virtual void visit(Triangle &t) = 0;
};

class Shape {
public:
    virtual void accept(Visitor &v) = 0;
};
