#ifndef UTIL_HPP
#define UTIL_HPP

#include <cassert>

// Taken from https://en.cppreference.com/w/cpp/error/assert
// Use (void) to silence unused warnings.
#define assertm(exp, msg) assert(((void)msg, exp))

#endif
