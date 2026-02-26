//
// Created by mpelo on 12/20/24.
// This multimap aggregates all values of keys that hash to the same value.
// It is the responsibility of the user to distinguish between the valid and not valid values.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_AGGREGATINGMULTIMAP_HPP
#define HIERARCHICALRELATIVELEMPELZIV_AGGREGATINGMULTIMAP_HPP

#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <random>

#include "HashFunction.hpp"

/**
 * Create an adjacency list of values of type V.
 * @tparam V the type of the values of the list
 * @param n the initial capacity of the list
 * @return the adjacency list (a pointer).
 */
template <typename V>
void *create_adj_list(size_t n) {
    void *adj_list = malloc(2*sizeof(unsigned int) + n * sizeof(V));
    if (adj_list == nullptr)
        throw std::runtime_error("Not enough space for adjacency list!!!");
    unsigned int *adj_list_params = reinterpret_cast<unsigned int*>(adj_list);
    adj_list_params[0] = 0; //size
    adj_list_params[1] = n; //capacity;
    return adj_list;
}

/**
 * Adds the value to the end of the adjacency list. Might reallocate,
 * thus the address to the pointer is required.
 * @tparam V the type of the values
 * @param adj_list a pointer to the adjacency list
 * @param value the value to add
 */
template <typename V>
void push_back_adj_list(void **adj_list, V value) {
    unsigned int *ptr = reinterpret_cast<unsigned int*>(*adj_list);
    unsigned int size = ptr[0];
    unsigned int capacity = ptr[1];
    if (size == capacity) {
        //realloc
        capacity = capacity << 1; //*2
        void *new_ptr = realloc(*adj_list, 2 * sizeof(unsigned int) + capacity * sizeof(V));
        if (new_ptr == nullptr)
            throw std::runtime_error("Not enough memory for adjacency list when growing!!!");
        *adj_list = new_ptr;
        ptr = reinterpret_cast<unsigned int *>(new_ptr);
        ptr[1] = capacity;
    }
    V *values = reinterpret_cast<V*>(ptr + 2);
    values[size] = value;
    ptr[0] = size + 1;
}

/**
 * Frees the adjacency list. The pointer should be
 * set to null afterwards by the user.
 * @param adj_list the adjacency list
 */
template <typename V>
void delete_adj_list(void *adj_list) {
    free(adj_list);
}

/**
 * Returns the number of elements in the adjacency list.
 * @tparam V the tupe of the elements
 * @param adj_list the adjacency list
 * @return the number of elements in adj_list
 */
template <typename V>
unsigned int size_adj_list(void *adj_list) {
    return (reinterpret_cast<unsigned int*>(adj_list))[0];
}
/**
 * Returns the current capacity of the adjacency list.
 * @tparam V the tupe of the elements
 * @param adj_list the adjacency list
 * @return the number of elements in adj_list
 */
template <typename V>
unsigned int capacity_adj_list(void *adj_list) {
    return (reinterpret_cast<unsigned int*>(adj_list))[1];
}
/**
 * Returns a pointer to the array of values of the adjacency list.
 * @tparam V the type of the values
 * @param adj_list the list
 * @return a pointer to an array with the values of the list
 */
template <typename V>
V *values_adj_list(void *adj_list) {
    return reinterpret_cast<V*>(reinterpret_cast<unsigned int*>(adj_list) + 2);
}

//Types: K for key, V for value
//K must be hashable (the user needs to provide a hash function)
template <typename K, typename V>
class AggregatingMultimap {
public:
    const size_t MIN_CAPACITY_BUCKET = 2;
    size_t size;

    AggregatingMultimap(size_t n, HashFunction<K>* hash_fn, bool (*equals_fn)(K key1, K key2))
            : size { n }
            , hash_table { new void*[n] } //Throws if there is not enough memory
            , size_is_two_pow { (n & (-n)) == n } //two's complement magic
            , hash_function { hash_fn }
            , equals { equals_fn }
    {
        for (size_t i = 0; i < size; i++)
            hash_table[i] = nullptr;
    }


    void delete_hash_table() {
        if (hash_table == nullptr) return; //It has been moved
        for (size_t i = 0; i < size; i++) {
            if (hash_table[i] != nullptr) {
                delete_adj_list<V>(hash_table[i]);
            }
        }
        delete[] hash_table;
    }

    ~AggregatingMultimap() {
        delete_hash_table();
    }

    AggregatingMultimap(const AggregatingMultimap& other) = delete;   //We don't want a copy constructor
    AggregatingMultimap& operator=(const AggregatingMultimap& other) = delete; //We don't want a copy assignment operator

    AggregatingMultimap(AggregatingMultimap&& other) noexcept
            : size(other.size)
            , hash_table(other.hash_table)
            , size_is_two_pow(other.size_is_two_pow)
            , hash_function(other.hash_function)
            , equals(other.equals)
    {       //Move constructor
        other.hash_table = nullptr; //other is a temp rvalue and will be destroyed
    }

    AggregatingMultimap& operator=(AggregatingMultimap&& other) noexcept { //Move assignment, can be used to re-initialize
        if (&other == this)
            return *this;
        //Delete hash table
        delete_hash_table();
        size = other.size;
        hash_table = other.hash_table;
        other.hash_table = nullptr;
        size_is_two_pow = other.size_is_two_pow;
        hash_function = other.hash_function;
        equals = other.equals;
        return *this;
    }

    void insert_copy(K key, V value) {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        void* adj = hash_table[hash_value];
        if (adj == nullptr) {
            hash_table[hash_value] = create_adj_list<V>(MIN_CAPACITY_BUCKET);
        }
        push_back_adj_list(&hash_table[hash_value], value);
    }

    const std::pair<unsigned int, V*> find(K key) {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        void *adj_list = hash_table[hash_value];
        if (adj_list == nullptr) {
            return std::pair<unsigned int, V*>(0, nullptr);
        } else {
            return std::pair<unsigned int, V*>(size_adj_list<V>(adj_list), values_adj_list<V>(adj_list));
        }
    }

private:
    AggregatingMultimap();

    void **hash_table;
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

#endif //HIERARCHICALRELATIVELEMPELZIV_AGGREGATINGMULTIMAP_HPP
