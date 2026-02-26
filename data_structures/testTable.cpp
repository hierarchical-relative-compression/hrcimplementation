//
// Created by mpelo on 12/17/24.
//
#include "NoRehashingHashMap.hpp"
#include "TabulationHashing.hpp"
#include <cstdlib>
#include <iostream>

size_t hash_int64(size_t x) {
    return x;
}

bool equals(size_t x1, size_t x2) {
    return x1 == x2;
}

void testuniv() {
    UniversalMultiplyShift64 fn = UniversalMultiplyShift64(3);
    NoRehashingHashMap<size_t, size_t> table = NoRehashingHashMap<size_t, size_t>(4, &fn, equals);
    table.insert_copy(0, 29ull);
    table.insert_copy(1, 3ull);
    table.insert_copy(0, 8ull);
    size_t deff = -1;
    std::cout << table.find((size_t)5, &deff) << std::endl;
}

int main(int argc, char *argv[]) {
    uint64_t rand = 3;
    TabulationHashing40 hash(3);
    uint64_t key1 = 0x00000000FF0FF000;
    uint64_t key2 = 0x000000FFBBCDEB38;
    //std::cout << hash.hash(key1) << std::endl;
    std::cout << hash.hash(key2) << std::endl;
}