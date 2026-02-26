//
// Created by mpelo on 2/11/25.
//

#include "test_agg_multimap.hpp"
#include "AggregatingMultimap.hpp"
#include "HashFunction.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <random>

bool check_all(AggregatingMultimap<uint64_t, uint64_t>& table, const std::set<uint64_t>& model) {
    for (size_t key : model) {
        std::pair<unsigned int, uint64_t*> pair = table.find(key);
        if (pair.first == 0) return false;
    }
    return true;
}

int test_agg_multimap(const std::vector<uint64_t>& elements, uint64_t param) {
    size_t n = elements.size();
    UniversalMultiplyShift64 hash(param);
    AggregatingMultimap<uint64_t, uint64_t> table(n, &hash, equals_size_t);
    std::set<uint64_t> model;
    //Generate random values, we mostly care about the keys
    std::ranlux48_base gen { std::random_device{}() };
    for (int i = 0; i < n; i++) {
        size_t element = elements[i];
        table.insert_copy(element, gen());
        model.insert(element);
        if (!check_all(table, model)) {
            return i;
        }
    }
    return -1;
}

bool check_adj_list(void *adj_list, const std::vector<uint64_t>& elements, int i) {
    size_t *values = values_adj_list<size_t>(adj_list);
    for (int j = 0; j < i; j++) {
        if (values[j] != elements[j]) {
            return false;
        }
    }
    return true;
}

int test_adj_list(const std::vector<uint64_t>& elements) {
    void *adj_list = create_adj_list<size_t>(2);
    int n = elements.size();
    for (int i = 0; i < n; i++) {
        push_back_adj_list(&adj_list, elements[i]);
        if (!check_adj_list(adj_list, elements, i)) {
            return i;
        }
    }
    return -1;
}