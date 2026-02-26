//
// Created by mpelo on 10/30/24.
//

#include "PartialSums.hpp"

//Zeros constructor
FenwickTree::FenwickTree(size_t n) : PartialSums(n) {
    size = n;
    arr = (size_t *) std::calloc(size, sizeof(size_t)); //zero alloc
    //No need to make_fenwick, since all the elements are zero
}

FenwickTree::FenwickTree(size_t value, size_t n) : PartialSums(n) {
    size = n;
    arr = (size_t *) std::malloc(size * sizeof(size_t));
    for (size_t i = 0; i < size; i++)
        arr[i] = value;
    make_fenwick();
}

FenwickTree::FenwickTree(const std::vector<size_t>& init) : PartialSums(init) {
    reset(init);
}

void FenwickTree::reset(const std::vector<size_t>& init) {
    size = init.size();
    arr = (size_t *) std::malloc(size*sizeof(size_t));
    std::copy(init.begin(), init.end(), arr);
    make_fenwick();
}

void FenwickTree::make_fenwick() {
    //To make the Fenwick tree F, we copy each value of F to the next range that needs it
    for (size_t z = 0; z < size; z++) {
        size_t next = z | (z + 1);
        if (next < size && next > z) { //The second check is necessary in case of overflow
            arr[next] += arr[z];
        }
    }
}

FenwickTree::~FenwickTree() {
    free(arr);
}

size_t FenwickTree::query(size_t i) {
    size_t acc = arr[i];
    size_t new_i = i - ((i & ~(i + 1)) + 1); //i - 2^rmz(i)
    while (new_i < i) { //The powers of 2 minus 1 contain the full partial sum. Thus we don't need to look further
        i = new_i;
        acc += arr[i];
        new_i = i - ((i & ~(i + 1)) + 1);
    }
    return acc; //Wrong until we do the initialization correctly
}

void FenwickTree::update(size_t pos, int64_t delta) {
    arr[pos] += delta;
    size_t new_pos = pos | (pos + 1);
    while (new_pos > pos && new_pos < size) {
        pos = new_pos;
        arr[pos] += delta;
        new_pos = pos | (pos + 1);
    }
}

/**
 * We can retrieve the original array in O(N) time
 * @return the current array of elements
 */
std::vector<size_t> FenwickTree::retrieve_array() {
    std::vector<size_t> ret(arr, arr + size); //Copy of the Fenwick tree
    for (size_t i = 0; i < size; i++) {
        //Subtract the element in the Fenwick array from the next range that includes it
        size_t next = i | (i + 1);
        if (next < size && next > i) { //The second check is necessary in case of overflow
            ret[next] -= arr[i];
        }
    }
    return ret;
}

