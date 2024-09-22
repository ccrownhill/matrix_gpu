#include <cmath>
#include <cstdint>
#include <iostream>

#define ANGLE_BITS 17

// FPGA angles have values representing equal spacing between 0 and 2pi
uint16_t fpga_angle(double rad_angle)
{
    // Normalize to [0, 2pi)
    while (rad_angle < 0) {
        rad_angle += 2*M_PI;
    }
    while (rad_angle >= 2*M_PI) {
        rad_angle -= 2*M_PI;
    }

    uint16_t fpga_angle = std::floor(rad_angle * std::pow(2, ANGLE_BITS) / (2*M_PI) + 0.5);
    return fpga_angle;
}

int main()
{
    for (int i = 0; i < ANGLE_BITS; i++) {
        double arg = std::pow(2, -1 * i);
        uint16_t angle = fpga_angle(std::atan(arg));
        std::cout << std::dec << i << ": "
                  << std::hex << static_cast<unsigned int>(angle) << "\n";
    }
}
