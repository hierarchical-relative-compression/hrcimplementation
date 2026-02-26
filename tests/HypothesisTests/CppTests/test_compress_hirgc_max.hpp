//
// Created by mpelo on 7/22/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_TEST_COMPRESS_HIRGC_MAX_HPP
#define HIERARCHICALRELATIVELEMPELZIV_TEST_COMPRESS_HIRGC_MAX_HPP

#include <string>
int64_t test_compress_hirgcmax(const std::string& ref_fasta, const std::string& target_fasta, uint64_t random);

int64_t test_hirgcmax_against_old(const std::string& ref_fasta, const std::string& target_fasta, uint64_t random);

#endif //HIERARCHICALRELATIVELEMPELZIV_TEST_COMPRESS_HIRGC_MAX_HPP
