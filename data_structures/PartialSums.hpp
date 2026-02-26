//
// Created by mpelo on 10/30/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_PARTIALSUMS_H
#define HIERARCHICALRELATIVELEMPELZIV_PARTIALSUMS_H

#include <vector>
#include <memory>

class PartialSums {
public:
    //Initialize to the given vector
    PartialSums(std::vector<size_t> arr) {}
    //Initialize to zeros
    PartialSums(size_t size) {}
    //Destructor
    virtual ~PartialSums() {}
    virtual size_t query(size_t i) = 0;
    virtual void update(size_t pos, int64_t delta) = 0;
};

class FenwickTree : public PartialSums {
private:
    size_t size;
    size_t *arr;
    void make_fenwick(); //Computes the fenwick array on arr, in-place
public:
    /**
     * Construct a Fenwick tree from an array of n zeros.
     * @param n the length of the array.
     */
    FenwickTree(size_t n);
    /**
     * Construct a Fenwick tree from an array of @n elements set to @value.
     * @param value the initial values of the array.
     * @param n the length of the array
     */
    FenwickTree(size_t value, size_t n);
    /**
     * Construct a Fenwick tree from a given initial array.
     * @param vec the initial array.
     */
    FenwickTree(const std::vector<size_t>& vec);
    void reset(const std::vector<size_t>& vec);
    ~FenwickTree();

    size_t query(size_t i) override;
    void update(size_t pos, int64_t delta) override;
    std::vector<size_t> retrieve_array();
};
#endif //HIERARCHICALRELATIVELEMPELZIV_PARTIALSUMS_H
