#include <cmath>
#include <iostream>

void cordic_rm_cc(double x_in, double y_in, double z_in)
{

    for (int i = 0; i < 100; i++) {
        std::cout << "i: " << i
                  << " \tx: " << x_in
                  << " \ty: " << y_in
                  << " \tz: " << z_in << "\n";

        double alpha = std::atan(std::pow(2, -1 * i));
        double sigma = (z_in >= 0) ? 1 : -1;
        double x_out = x_in - sigma * std::pow(2, -1 * i) * y_in;
        double y_out = y_in + sigma * std::pow(2, -1 * i) * x_in;
        double z_out = z_in - sigma * alpha;

        x_in = x_out;
        y_in = y_out;
        z_in = z_out;
    }
}

void cordic_vm_hc(double x_in, double y_in, double z_in)
{
    bool double_iter = true;

    for (int i = 1; i < 100; i++) {
        std::cout << "i: " << i
                  << " \tx: " << x_in
                  << " \ty: " << y_in
                  << " \tz: " << z_in << "\n";

        double alpha = std::atanh(std::pow(2, -1 * i));
        double sigma = (y_in * x_in >= 0) ? -1 : 1;
        double x_out = x_in + sigma * std::pow(2, -1 * i) * y_in;
        double y_out = y_in + sigma * std::pow(2, -1 * i) * x_in;
        double z_out = z_in - sigma * alpha;

        x_in = x_out;
        y_in = y_out;
        z_in = z_out;

        if (i == 4 || i == 13 || i == 40) {
            i = (double_iter) ? i - 1 : i;
            double_iter = !double_iter;
        }
    }
}

int main()
{
    cordic_vm_hc(1.25, 0.75, 0);
}
