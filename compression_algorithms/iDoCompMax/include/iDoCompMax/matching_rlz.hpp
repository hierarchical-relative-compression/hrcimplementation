//
// Created by mpelo on 8/6/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_MATCHING_RLZ_HPP
#define HIERARCHICALRELATIVELEMPELZIV_MATCHING_RLZ_HPP

#include <vector>
#include <cstddef>

struct match {
    size_t pos;
    size_t len;
    char mismatch;
};

std::vector<struct match> rlz_match(char *ref, int *ref_sa, char *target, size_t n_ref, size_t n_target);

#endif //HIERARCHICALRELATIVELEMPELZIV_MATCHING_RLZ_HPP
