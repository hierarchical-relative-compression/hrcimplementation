//
// Created by mpelo on 12/17/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHMAP_HPP
#define HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHMAP_HPP

#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <cstdint>

#include "HashFunction.hpp"

//Types: K for key, V for value
//K must be hashable (the user needs to provide a hash function)
template <typename K, typename V>
class NoRehashingHashMap {
public:
    size_t size;
    NoRehashingHashMap(size_t n, HashFunction<K>* hash_fn, bool (*equals_fn)(K key1, K key2))
        : size { n }
        , hash_table { new std::vector<pair<K, V>>[n] } //Initializes all the vectors with the default constructor
        , size_is_two_pow { (size & (-size)) == size } //two's complement magic
        , hash_function { hash_fn }
        , equals { equals_fn }
    {}

    ~NoRehashingHashMap() {
        delete[] hash_table;
    }

    NoRehashingHashMap(const NoRehashingHashMap& other) = delete;   //We don't want a copy constructor
    NoRehashingHashMap& operator=(const NoRehashingHashMap& other) = delete; //We don't want a copy assignment operator

    NoRehashingHashMap(NoRehashingHashMap&& other) noexcept
        : size(other.size)
        , hash_table(other.hash_table)
        , size_is_two_pow(other.size_is_two_pow)
        , hash_function(other.hash_function)
        , equals(other.equals)
    {       //Move constructor
        other.hash_table = nullptr; //other is a temp rvalue and will be destroyed
    }

    NoRehashingHashMap& operator=(NoRehashingHashMap&& other) noexcept { //Move assignment, can be used to re-initialize
        if (&other == this)
            return *this;
        delete[] hash_table;
        size = other.size;
        hash_table = other.hash_table;
        other.hash_table = nullptr;
        size_is_two_pow = other.size_is_two_pow;
        hash_function = other.hash_function;
        equals = other.equals;
        return *this;
    }

    bool insert_copy(K key, V value) {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        std::vector<pair<K, V>>& adj = hash_table[hash_value];
        pair<K, V> *elem = adj.data();
        size_t len = adj.size();
        for (size_t i = 0; i < len; i++) {
            if (equals(elem->key, key)) {
                elem->value = value;
                return true;
            }
            elem++;
        }
        adj.emplace_back(key, value);
        return false;
    }

    V* find(K key, V* default_value) {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        std::vector<pair<K, V>>& adj = hash_table[hash_value];
        pair<K, V> *elem = adj.data();
        size_t len = adj.size();
        for (size_t i = 0; i < len; i++) {
            if (equals(elem->key, key)) {
                return &(elem->value);
            }
            elem++;
        }
        return default_value;
    }

private:
    NoRehashingHashMap();

    std::vector<pair<K, V>> *hash_table;
    bool size_is_two_pow;
    //Hash and equals functions
    HashFunction<K> *hash_function;
    bool (*equals)(K key1, K key2);

    size_t opt_modulus(size_t hash_value) {
        if (size_is_two_pow) {
            return hash_value & (size - 1);
        } else {
            return hash_value % size;
        }
    }
};




#endif //HIERARCHICALRELATIVELEMPELZIV_NOREHASHINGHASHMAP_HPP
