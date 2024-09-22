#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <iostream>
#include <string>
#include <stack>

// TODO get rid of getters and setters by making this a struct to directly
// access member data
class Context {
public:
    Context()
        : string_reg {""},
          free_regs {{7, 6, 5, 4}},
          used_regs {},
          x_reg {-1}, y_reg {-1}
    {}
    ~Context() {}

    int alloc_reg();
    int free_reg();
    int get_last_used_reg() const;

    void set_string_reg(std::string s);
    std::string get_string_reg() const;
    std::string get_cur_out_reg();

    void set_row(int r);
    int get_row() const;

    void set_x_reg(int r);
    int get_x_reg() const;
    void set_y_reg(int r);
    int get_y_reg() const;
private:
    int row;
    std::string string_reg;
    std::stack<int> free_regs;
    std::stack<int> used_regs;
    int x_reg;
    int y_reg;

    int stack_exchange(std::stack<int> &from, std::stack<int> &to);
};

#endif
