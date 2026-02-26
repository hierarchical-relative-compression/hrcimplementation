//
// Created by mpelo on 2/12/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HIRGC64BITSCHAINING_HPP
#define HIERARCHICALRELATIVELEMPELZIV_HIRGC64BITSCHAINING_HPP

#include "HashFunction.hpp"
#include <memory>

template <typename K, typename V>
class HiRGC64bitsChaining {
    static const size_t END_ARRAY_MASK = 1ULL << 63;
    static const size_t MIN_BUCKET = 2;
public:
    size_t capacity;
    size_t size;
    V default_value;

    HiRGC64bitsChaining() {}

    HiRGC64bitsChaining(size_t max_n_tuples, HashFunction<K> *fn, V default_value, const char *seq, int k)
            : capacity { std::bit_ceil(max_n_tuples) }
            , size { 0LL }
            , default_value { default_value }
            , hash_table { std::make_unique<V*[]>(capacity) }
            , capacity_is_two_pow { (capacity & (-capacity)) == capacity } //two's complement magic
            , hash_function { fn }
            , entries { std::make_unique<V[]>(max_n_tuples) }
            , entries_i { 0 }
            , seq_DNA { seq }
            , k { k }
    {
        for (size_t i = 0; i < capacity; i++)
            hash_table[i] = nullptr;
    }

    ~HiRGC64bitsChaining() {
        if (hash_table != nullptr) { //The hash table could have been moved
            for (size_t i = 0; i < capacity; i++) {
                if (hash_table[i] != nullptr)
                    delete[] hash_table[i];
            } //The hash table itself it's deleted by unique_ptr
        }
    }

    HiRGC64bitsChaining(const HiRGC64bitsChaining& other) = delete;   //We don't want a copy constructor
    HiRGC64bitsChaining& operator=(const HiRGC64bitsChaining& other) = delete; //We don't want a copy assignment operator

    HiRGC64bitsChaining(HiRGC64bitsChaining&& other) noexcept //Move constructor
            : capacity { other.capacity }
            , size { other.size }
            , default_value { other.default_value }
            , hash_table { std::move(other.hash_table) }
            , capacity_is_two_pow { other.capacity_is_two_pow }
            , hash_function { other.hash_function }
            , entries { std::move(other.entries) }
            , entries_i { 0 }
            , seq_DNA { other.seq_DNA }
            , k { k }
    {}

    HiRGC64bitsChaining& operator=(HiRGC64bitsChaining<K, V>&& other) noexcept { //Move assignment, can be used to re-initialize
        if (&other == this)
            return *this;
        capacity = other.capacity;
        size = other.size;
        default_value = other.default_value;
        hash_table = std::move(other.hash_table);
        capacity_is_two_pow = other.capacity_is_two_pow;
        hash_function = other.hash_function;
        other.hash_function = nullptr;
        entries = std::move(other.entries);
        entries_i = other.entries_i;
        seq_DNA = other.seq_DNA;
        k = other.k;
        return *this;
    }

    V insert_copy(K tuple, V position) {
        size_t hash_value = opt_modulus(hash_function->hash(tuple));
        V *chain = hash_table[hash_value];
        if (chain != nullptr) {
            //Iterate through the chain to see if there is a matching tuple (reassignment)
            bool found = false;
            size_t i = iteration_chain(chain, tuple, &found);
            if (found) {
                //Reassignment
                V old_entries_pos = chain[i];
                if (old_entries_pos >= END_ARRAY_MASK) {
                    old_entries_pos -= END_ARRAY_MASK;
                    entries[position] = old_entries_pos;
                    chain[i] = position + END_ARRAY_MASK;
                } else {
                    entries[position] = old_entries_pos;
                    chain[i] = position;
                }
                entries_i++;
                return old_entries_pos;
            }
            //Not found: we reached the end of the chain, and have to add a value.
            //If a chain is not completely full, its size is not a power of two, because at least one element is always
            //inserted after the doubling
            if ((i & (i - 1)) == 0) {
                V *new_chain = static_cast<V*>(realloc(chain, (i << 1)*sizeof(V)));
                if (new_chain == nullptr)
                    throw std::runtime_error("Not enough memory for chain list when growing!!!");
                hash_table[hash_value] = new_chain;
                chain = new_chain;
            }
            //Add new element after the last, changing the mask
            chain[i] = position + END_ARRAY_MASK;
            chain[i-1] -= END_ARRAY_MASK;
            entries[position] = default_value;
            entries_i++;
            size++;
            return default_value;
        } else {
            //Create new bucket with at least MIN_BUCKET of capacity and one element
            hash_table[hash_value] = static_cast<V*>(malloc(MIN_BUCKET * sizeof(V)));
            *hash_table[hash_value] = position + END_ARRAY_MASK;
            size++;
            //Add entry
            entries[position] = default_value;
            entries_i++;
            return default_value;
        }
    }

    size_t iteration_chain(V* chain, K key, bool *found) {
        size_t i = 0;
        bool last_value = false;
        do {
            V old_entries_pos = chain[i];
            if (old_entries_pos >= END_ARRAY_MASK) {
                last_value = true;
                old_entries_pos -= END_ARRAY_MASK;
            }
            size_t val_tup = make_tuple(old_entries_pos);
            if (val_tup == key) {
                *found = true;
                return i;
            }
            i++;
        } while (!last_value);
        *found = false;
        return i;
    }

    void rearrange() {
        //Put all the chains in a contiguous array in memory.
        std::unique_ptr<V[]> new_entries = std::make_unique<V[]>(entries_i + size); //We have one separator per chain
        V j = 0;
        //Scan the hash table to find the chains
        //This can be time inefficient, but we save memory by avoiding an iterator on the hash table
        for (size_t i = 0; i < capacity; i++) {
            if (hash_table[i] != nullptr) {
                //new chain, explore it
                V *chain = hash_table[i];
                size_t chain_i = 0;
                bool last_value = false;
                do {
                    size_t entries_pos = chain[chain_i];
                    if (entries_pos >= END_ARRAY_MASK) {
                        last_value = true;
                        entries_pos -= END_ARRAY_MASK;
                        //Change the pointer of the hash table to the new entries position
                        chain[chain_i] = j + END_ARRAY_MASK;
                    } else {
                        chain[chain_i] = j;
                    }
                    do {
                        new_entries[j++] = entries_pos;
                        entries_pos = entries[entries_pos];
                    } while (entries_pos != default_value);
                    //Introduce a separator
                    new_entries[j++] = default_value;
                    chain_i++;
                } while (!last_value);
            }
        }
        //Substitute the old entries array by the new one
        entries = std::move(new_entries);
    }

    V *find(K key) const {
        size_t hash_value = opt_modulus(hash_function->hash(key));
        V *chain = hash_table[hash_value];
        if (chain == nullptr) {
            return nullptr;
        }
        size_t chain_i = 0;
        bool last_value = false;
        size_t entries_pos;
        do {
            entries_pos = chain[chain_i];
            if (entries_pos >= END_ARRAY_MASK) {
                last_value = true;
                entries_pos -= END_ARRAY_MASK;
            }
            size_t val_tup = make_tuple(entries[entries_pos]);
            if (val_tup == key) {
                return &entries[entries_pos];
            }
            chain_i++;
        } while (!last_value);
        return nullptr;
    }

    size_t make_tuple(V seq_pos) const {//Check if this entry is a collision
        //Compute the tuple at seq_pos
        size_t val_tup = 0;
        for (int i = 0; i < k; i++) {
            val_tup += (static_cast<size_t>(seq_DNA[seq_pos + i]) << (2 * i));
        }
        return val_tup;
    }

    V *raw() {
        return hash_table.get();
    }

private:
    std::unique_ptr<V*[]> hash_table;
    bool capacity_is_two_pow;
    //Hash and equals functions
    HashFunction<K> *hash_function;
    //vector of entries
    std::unique_ptr<V[]> entries;
    //Index on the entries
    size_t entries_i { 0 };
    //Sequence of DNA that it will be matched against
    const char *seq_DNA;
    //Size of the keys (the tuples)
    int k;

    size_t opt_modulus(size_t hash_value) const {
        if (capacity_is_two_pow) {
            return hash_value & (capacity - 1);
        } else {
            return hash_value % capacity;
        }
    }

    uint64_t index_ch(const char c) const {
        switch (c) {
            case 'A':
                return 0;
            case 'C':
                return 1;
            case 'G':
                return 2;
            case 'T':
                return 3;
            default:
                return 4;
        }
    }
};

#endif //HIERARCHICALRELATIVELEMPELZIV_HIRGC64BITSCHAINING_HPP
