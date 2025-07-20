#ifndef ADD_H
#define ADD_H

class Add {
    private:
        int first_operand;
        int second_operand;
    public:
        Add(int a, int b);
        int get_sum();
};

#endif