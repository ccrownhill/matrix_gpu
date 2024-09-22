#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include <vector>
#include <cmath>

//benchmarks parameters
#define testing_disp 0
#define image_num 300

#define Width 1920
#define Height 1080
const double x_rot = 0.0, y_rot = 0.0, z_rot = 0.0;
const double xsin = sin(x_rot), xcos = cos(x_rot), ysin = sin(y_rot), ycos = cos(y_rot), zsin = sin(z_rot), zcos = cos(z_rot);

uint32_t pixel_col(double z) { // note: z is in the range [0, 1]
    uint32_t rgb;
    uint32_t z_int;
    uint32_t lookup_table[64] = {
        0x8000ff,
        0x770dff,
        0x6f19ff,
        0x6726fe,
        0x5f33fe,
        0x573ffd,
        0x4f4bfc,
        0x4757fb,
        0x3f63fa,
        0x376ff9,
        0x2f7af7,
        0x2685f5,
        0x1e90f4,
        0x169af2,
        0x0ea4f0,
        0x06aded,
        0x02b7eb,
        0x0abfe8,
        0x12c7e6,
        0x1acfe3,
        0x22d6e0,
        0x2adddd,
        0x33e3da,
        0x3be8d6,
        0x43edd3,
        0x4bf2cf,
        0x53f5cb,
        0x5bf9c7,
        0x63fbc3,
        0x6bfdbf,
        0x73febb,
        0x7bffb7,
        0x84ffb2,
        0x8cfead,
        0x94fda9,
        0x9cfba4,
        0xa4f99f,
        0xacf59a,
        0xb4f295,
        0xbced90,
        0xc4e88a,
        0xcce385,
        0xd4dd80,
        0xddd67a,
        0xe5cf74,
        0xedc76f,
        0xf5bf69,
        0xfdb763,
        0xffad5d,
        0xffa457,
        0xff9a51,
        0xff904b,
        0xff8545,
        0xff7a3f,
        0xff6f39,
        0xff6333,
        0xff572c,
        0xff4b26,
        0xff3f20,
        0xff3319,
        0xff2613,
        0xff190d,
        0xff0d06,
        0xff0000
    };
    z*=64;
    z_int = uint32_t(z);

    if (z_int > 64 || z < 0) 
        rgb = 0xffffff;
    else if (z_int == 64) {
        rgb = 0xff0000;
    }
    else if (z_int == 0) 
        rgb = 0x8000ff;
    else {
        rgb = lookup_table[z_int];    
    }
    return rgb;
}