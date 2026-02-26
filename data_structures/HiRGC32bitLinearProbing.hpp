//
// Created by mpelo on 2/26/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HIRGC32BITLINEARPROBING_HPP
#define HIERARCHICALRELATIVELEMPELZIV_HIRGC32BITLINEARPROBING_HPP

#include "HashFunction.hpp"
#include <cstddef>
#include <memory>
#include <iostream>
#include <KeyValueArray.hpp>

/*
#define index(hash_table, i) hash_table + (i) * 10
#define key(hash_table, i) hash_table + (i) * 10
#define value(hash_table, i) hash_table + (i) * 10 + 5
#define next(hash_table) hash_table + 10
*/

struct Entry {
    uint64_t key;
    uint64_t value;
};

#define KEYMAX 0xFFFFFFFFFFFFULL        //6 bytes (40 bits=5 bytes + 1 bit for max value)
#define VALPTR_MAX 0xFFFFFFFFFFULL    //5 bytes
#define VALMAX UINT32_MAX

class HiRGC32bitLinearProbing {
public:
    size_t capacity;
    size_t modulus;
    uint32_t size;
    KeyValueArray<size_t, size_t> hash_table;
    HashFunction<uint64_t> *hash_function { nullptr };
    std::unique_ptr<uint32_t[]> entries;
    uint32_t entries_i { 0 };

    HiRGC32bitLinearProbing() {}

    HiRGC32bitLinearProbing(size_t max_n_tuples, HashFunction<uint64_t> *fn)
        : capacity { std::bit_ceil(max_n_tuples << 1) } //twice as big
        , modulus { capacity - 1 }
        , size { 0 }
        , hash_table { 6, 5, capacity }
        , hash_function { fn }
        , entries { std::make_unique<uint32_t[]>(max_n_tuples) }
        , entries_i { 0 }
    {
        for (size_t i = 0; i < capacity; i++) {
            hash_table.set(i, KEYMAX, VALPTR_MAX);
        }
    }

    ~HiRGC32bitLinearProbing() {
        //Entries is deleted automatically, and the destructor for hash_table is called
    }

    HiRGC32bitLinearProbing(const HiRGC32bitLinearProbing& other) = delete;  //We don't want a copy constructor
    HiRGC32bitLinearProbing& operator=(const HiRGC32bitLinearProbing& other) = delete;  //We don't want a copy assignment operator

    HiRGC32bitLinearProbing(HiRGC32bitLinearProbing&& other) noexcept
        : capacity { other.capacity }
        , modulus { capacity - 1 }
        , size { other.size }
        , hash_table { std::move(other.hash_table) }
        , hash_function { other.hash_function }
        , entries { std::move(other.entries) }
        , entries_i { other.entries_i }
    {
        other.entries = nullptr;  //To avoid the destructor freeing this memory
    }

    HiRGC32bitLinearProbing& operator=(HiRGC32bitLinearProbing&& other) noexcept { //Move assignment
        if (&other == this)
            return *this;
        capacity = other.capacity;
        modulus = capacity - 1;
        size = other.size;
        hash_table = std::move(other.hash_table);
        hash_function = other.hash_function;
        entries = std::move(other.entries);
        entries_i = other.entries_i;
        other.entries = nullptr;
        return *this;
    }

    uint32_t insert_copy(uint64_t key, uint32_t position) {
        size_t hash_value = hash_function->hash(key) & modulus; //Fast modulo
        //Linear probing
        KeyValEntry<size_t, size_t> e = hash_table.get(hash_value);
        while (e.key != KEYMAX && e.key != key) {
            //Linear probing: advance
            hash_value = (hash_value + 1) & modulus;
            e = hash_table.get(hash_value);
        }
        if (e.key == KEYMAX) {
            //The key was not found, and hash_value points to the first empty slot
            hash_table.set(hash_value,  key, position);
            entries[entries_i++] = VALMAX;
            size++;
        } else {
            //The key was found: reassignment
            hash_table.setValue(hash_value, position);
            entries[entries_i++] = e.value;
        }
        return e.value;
    }

    void rearrange() {
        //Put all the entries of the same key in a contiguous array in memory,
        //and update the "pointer" (value) in the hash table
        uint64_t new_size = static_cast<uint64_t>(entries_i) + static_cast<uint64_t>(size);
        std::unique_ptr<uint32_t[]> new_entries = std::make_unique<uint32_t[]>(new_size);
        uint64_t j = 0;
        for (size_t i = 0; i < capacity; i++) {
            if (hash_table.getKey(i) != KEYMAX) {
                //new chain, explore it
                uint64_t pos = hash_table.getValue(i);
                hash_table.setValue(i, j);
                do {
                    new_entries[j++] = pos;
                    pos = entries[pos];
                } while (pos != VALMAX);
                new_entries[j++] = VALMAX;
            }
        }
        //Substitute the old entries array by the new one
        entries = std::move(new_entries);
    }

    uint32_t *find(uint64_t key) const {
        size_t hash_value = hash_function->hash(key) & modulus;
        //Linear probing
        size_t table_key = hash_table.getKey(hash_value);
        while (table_key != KEYMAX && table_key != key) {
            //Linear probing: advance
            hash_value = (hash_value + 1) & modulus;
            table_key = hash_table.getKey(hash_value);
        }
        if (table_key == KEYMAX) {
            //Key not found
            return nullptr;
        } else {
            //Key was found. Look in entries and return that value
            return &entries[hash_table.getValue(hash_value)];
        }
    }
};

#endif //HIERARCHICALRELATIVELEMPELZIV_HIRGC32BITLINEARPROBING_HPP
