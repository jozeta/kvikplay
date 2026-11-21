#pragma once
// Test assertion that stays active in every build type. Plain assert() is
// compiled out under NDEBUG (Release), which made the suite unable to fail.
#include <cstdlib>
#include <iostream>

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << "CHECK failed: " #cond " (" << __FILE__ << ":"          \
                      << __LINE__ << ")" << std::endl;                           \
            std::exit(1);                                                        \
        }                                                                        \
    } while (0)
