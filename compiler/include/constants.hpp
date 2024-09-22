#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

//#define NOROT

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int PLOT_WIDTH = 512;
constexpr int PLOT_HEIGHT = 512;
constexpr int BLOCK_DIM = 8;
constexpr int MAX_INSTR = 256;
constexpr int NUM_THREADS = 16;
constexpr int MEM_SIZE = 512*512;

constexpr int NUM_BLOCKS = PLOT_WIDTH / BLOCK_DIM; // one program per pixel row
constexpr double EQUALITY_ERROR_MARGIN = 0.035;
constexpr double X_MAX = 5;
constexpr double X_MIN = -5;
constexpr double Y_MAX =
    static_cast<double>(PLOT_HEIGHT)/static_cast<double>(PLOT_WIDTH) * X_MAX;
constexpr double Y_MIN =
    static_cast<double>(PLOT_HEIGHT)/static_cast<double>(PLOT_WIDTH) * X_MIN;
constexpr double Z_MAX = 5;
constexpr double Z_MIN = -5;
constexpr int MIN_INFINITY = 0b1'1111111'00000'00000;
constexpr int NaN = 0b0111'1111'1000'0000'0000'0000'0000'0001;

#endif
