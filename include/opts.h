#pragma once

#include <cstdint>

struct Options {
    uint32_t n;
    uint32_t seed;
    bool debug;

  public:
    static Options parse(int argc, char** argv);
};
