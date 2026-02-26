//
// Created by mpelo on 2/11/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHSET_HPP
#define HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHSET_HPP

#include "HashFunction.hpp"

template <typename K, typename V>
class NoRehashingHashSet {
public:
    size_t capacity;
    V default_value;
    size_t size;

    NoRehashingHashSet(size_t n, HashFunction<K> *fn, bool (*equals_fn)(K key1, K key2), V default_value)
        : capacity { n }
        , hash_table { new V[n] }
        , capacity_is_two_pow { (capacity & (-capacity)) == capacity } //two's complement magic
        , hash_function { fn }
        , equals { equals_fn }
        , default_value { default_value }
        , size { 0LL }
        {
            for (size_t i = 0; i < n; i++)
                hash_table[i] = default_value;
        }

    ~NoRehashingHashSet() {
        delete[] hash_table;
    }

    NoRehashingHashSet(const NoRehashingHashSet& other) = delete;   //We don't want a copy constructor
    NoRehashingHashSet& operator=(const NoRehashingHashSet& other) = delete; //We don't want a copy assignment operator

    NoRehashingHashSet(NoRehashingHashSet&& other) noexcept //Move constructor
    : capacity(other.capacity)
    , hash_table(other.hash_table)
    , capacity_is_two_pow(other.capacity_is_two_pow)
    , hash_function { other.hash_function }
    , equals { other.equals }
    , default_value {other.default_value }
    , size { other.size }
    {
        other.hash_table = nullptr; //other is a temp rvalue and will be destroyed
    }

    NoRehashingHashSet& operator=(NoRehashingHashSet&& other) noexcept { //Move assignment, can be used to re-initialize
        if (&other == this)
            return *this;
        delete[] hash_table;
        capacity = other.capacity;
        hash_table = other.hash_table;
        other.hash_table = nullptr;
        capacity = other.capacity;
        hash_function = other.hash_function;
        equals = other.equals;
        default_value = other.default_value;
        size = other.size;
        return *this;
    }
    /**
     * Inserts the value at the position hash_function->hash(key). Overrides any previous value at that position.
     * @param key the key to insert the value at
     * @param value the value to associate hash(key) with
     * @return the previous associated value with hash(key)
     */
    V insert_copy(K key, V value) {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        V old = hash_table[hash_value];
        hash_table[hash_value] = value;
        if (old == default_value) {
            size++; //Otherwise, it's a reassignment
        }
        return old;
    }

    V find(K key) const {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        return hash_table[hash_value];
    }

    V *raw() {
        return hash_table;
    }

private:
    NoRehashingHashSet();

    V *hash_table;
    bool capacity_is_two_pow;
    //Hash and equals functions
    HashFunction<K> *hash_function;
    bool (*equals)(K key1, K key2);

    size_t opt_modulus(size_t hash_value) const {
        if (capacity_is_two_pow) {
            return hash_value & (capacity - 1);
        } else {
            return hash_value % capacity;
        }
    }
};

#endif //HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHSET_HPP
