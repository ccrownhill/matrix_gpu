#include <string>

#include <context.hpp>

void Context::set_row(int r)
{
    row = r;
}

int Context::get_row() const
{
    return row;
}

int Context::alloc_reg()
{
    return stack_exchange(free_regs, used_regs);
}

int Context::free_reg()
{
    return stack_exchange(used_regs, free_regs);
}

int Context::get_last_used_reg() const
{
    if (used_regs.empty()) {
        return -1;
    }

    return used_regs.top();
}

void Context::set_string_reg(std::string s)
{
    string_reg = s;
}

std::string Context::get_string_reg() const
{
    return string_reg;
}

std::string Context::get_cur_out_reg()
{
    if (string_reg == "") {
        return "r" + std::to_string(get_last_used_reg());
    } else {
        std::string out = string_reg;
        string_reg = "";
        return out;
    }
}

int Context::stack_exchange(std::stack<int> &from, std::stack<int> &to)
{
    if (from.empty()) {
        return -1;
    }
    int out = from.top();
    from.pop();
    to.push(out);
    return out;
}

void Context::set_x_reg(int r)
{
    x_reg = r;
}

int Context::get_x_reg() const
{
    return x_reg;
}

void Context::set_y_reg(int r)
{
    y_reg = r;
}

int Context::get_y_reg() const
{
    return y_reg;
}
