//
// Created by mpelo on 2/18/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_STRONGUNIVERSALHASHING_HPP
#define HIERARCHICALRELATIVELEMPELZIV_STRONGUNIVERSALHASHING_HPP

#include "HashFunction.hpp"
#include "utils/GlobalRandom.hpp"
#include <iostream>

class StrongUniversalHashing64 : public HashFunction<size_t> {
private:
    uint64_t seed;
    //It is initialized with a random long from the hardware (on random_device) or a specified value
    std::ranlux48_base gen;
    //Local random longs
    uint64_t a1, a2, b;
    int shift; //2**l
public:
    //Random seed
    StrongUniversalHashing64(int l) : StrongUniversalHashing64(GlobalRandom::get(), l) {}
    StrongUniversalHashing64(uint64_t random, int l)
        : seed { random }
        , gen { seed }
        , shift { 64 - l } {
        //std::cout << "Random seed: " << random << std::endl;
        //For sanity, we want parameters that are not 0 or 1
        while ((a1 = gen()) <= 1);
        while ((a2 = gen()) <= 1);
        while ((b = gen()) <= 1);
    }

    size_t hash(size_t key) const override {
        // hashes 64-bit x strongly universally into l<=32 bits
        // using the random seeds a1, a2, and b.
        return ((a1 + key) * (a2 + (key >> 32)) + b) >> shift;
    }
};


#endif //HIERARCHICALRELATIVELEMPELZIV_STRONGUNIVERSALHASHING_HPP
