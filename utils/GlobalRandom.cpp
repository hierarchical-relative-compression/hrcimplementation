//
// Created by mpelo on 3/20/25.
//
#include "include/utils/GlobalRandom.hpp"
#include <mutex>
#include <iostream>

#ifdef _MSC_VER
#define _UNUSED
#else
#define _UNUSED __attribute((unused))
#endif // _MSC_VER

namespace GlobalRandom {
    uint64_t global_seed { std::random_device{}() };
    std::ranlux48_base gen { global_seed };

    //Static block: for an implementation look at
    //https://stackoverflow.com/questions/19227664/whats-the-c-idiom-equivalent-to-the-java-static-block
    static void static_random();
    static int ye _UNUSED = (static_random(), 2);
    void static_random() {
        std::cout << "Global seed: " << global_seed << std::endl;
    }

    uint64_t get() {
        static std::mutex mut;
        mut.lock();
        uint64_t res = gen();
        mut.unlock();
        return res;
    }

    void set_global_seed(uint64_t seed) {
        global_seed = seed;
        gen.seed(seed);
        std::cout << "The seed was set to " << seed;
    }
}