//
// Created by mpelo on 2/13/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_TABULATIONHASHING_HPP
#define HIERARCHICALRELATIVELEMPELZIV_TABULATIONHASHING_HPP

#include "HashFunction.hpp"
#include <iostream>
#include <memory>
#include "utils/GlobalRandom.hpp"

class TabulationHashing40 : public HashFunction<size_t> {
private:

    typedef uint64_t hashsize;

    size_t seed; //Seed is initialized before gen
    std::ranlux48_base gen {};
    inline static int n_tables = 8;
    inline static int size_table = 32;
    inline static int n_rands = n_tables * size_table;

    std::unique_ptr<hashsize[]> tables;

#define VAL(table, byte) tables[ table * size_table + byte ]

    void fill_random() {
        for (int i = 0; i < n_rands; i++)
            tables[i] = static_cast<hashsize>(gen());
    }

public:
    TabulationHashing40() :
        seed { GlobalRandom::get() },
        gen { seed }, //It is initialized with a random long from the hardware (on random_device)
        tables { std::make_unique<hashsize[]>(n_tables * size_table) }
    {
        std::cout << "Random seed: " << seed << std::endl;
        fill_random();
    }
    TabulationHashing40(uint64_t param) :
        gen { param },
        tables { std::make_unique<hashsize[]>(n_tables * size_table) }
    {
        fill_random();
    }
    size_t hash(size_t key) const override {
        uint8_t mask_five = 0b00011111;
        hashsize res = 0;
        hashsize vals[8];
        for (int i = 0; i < 8; i++) {
            hashsize k = (key >> 5*i) & mask_five;
            vals[i] = VAL(i, k);
        }
        for (int i = 0; i < 8; i++) {
            res = res ^ vals[i];
        }
        return res;
    }
};

#endif //HIERARCHICALRELATIVELEMPELZIV_TABULATIONHASHING_HPP
