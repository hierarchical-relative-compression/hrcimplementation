//
// Created by mpelo on 12/20/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HASHFUNCTION_HPP
#define HIERARCHICALRELATIVELEMPELZIV_HASHFUNCTION_HPP
#include <random>
#include <stdexcept>
#include "utils/GlobalRandom.hpp"

template <typename K, typename V>
struct pair {
    K key;
    V value;
};

template <typename K>
class HashFunction {
public:
    virtual size_t hash(K key) const = 0;
};

class UniversalMultiplyShift64 : public HashFunction<size_t> {
private:
    //We have the same PRNG for all objects (should be fine)
    //It is initialized with a random long from the hardware (on random_device)
    inline static std::ranlux48_base gen { GlobalRandom::get() };
    //Local random long
    uint64_t a;
public:
    UniversalMultiplyShift64() {
        //We need an odd random value
        //Von Neumann sampling: we keep iterating until we get an odd value
        while (((a = gen()) & 1) == 0 || (a == 1));
    }
    UniversalMultiplyShift64(uint64_t param) {
        if (param % 2 == 0)
            throw std::runtime_error(std::string("Made universal multiply-shift with an even parameter!!!") + std::to_string(a));
        if (param == 1)
            throw std::runtime_error("Made universal multiply-shift with 1 as a parameter!!!");
        a = param;
    }
    size_t hash(size_t key) const override {
        return (key * a);
    }
};

bool equals_size_t(size_t key1, size_t key2);
#endif //HIERARCHICALRELATIVELEMPELZIV_HASHFUNCTION_HPP
