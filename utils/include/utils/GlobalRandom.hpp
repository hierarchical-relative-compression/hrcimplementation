//
// Created by mpelo on 3/20/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_GLOBALRANDOM_HPP
#define HIERARCHICALRELATIVELEMPELZIV_GLOBALRANDOM_HPP

#include <random>
#include <cstdint>

namespace GlobalRandom {
    extern uint64_t global_seed;
    extern std::ranlux48_base gen;
    uint64_t get();
    void set_global_seed(uint64_t seed);
}

#endif //HIERARCHICALRELATIVELEMPELZIV_GLOBALRANDOM_HPP
