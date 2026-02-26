//
// Created by mpelo on 8/6/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_TEST_IDOCOMPMAX_HPP
#define HIERARCHICALRELATIVELEMPELZIV_TEST_IDOCOMPMAX_HPP

#include <filesystem>
#include <vector>
#include "iDoCompMax/matching_rlz.hpp"
#include "iDoCompMax/iDoCompMaxPostprocessing.hpp"

std::vector<struct match> test_compress_idocompmax(const std::string& ref, const std::string& target);

int test_idocompmax_parse_subs(const std::string& ref, const std::string& target,
                               std::vector<struct match>& matches, std::vector<struct substitution>& subs);

int test_idocompmax_insertions_decomp(const std::string& ref, const std::string& target);


#endif //HIERARCHICALRELATIVELEMPELZIV_TEST_IDOCOMPMAX_HPP
