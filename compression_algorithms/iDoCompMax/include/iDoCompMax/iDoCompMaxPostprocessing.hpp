//
// Created by mpelo on 8/7/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXPOSTPROCESSING_HPP
#define HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXPOSTPROCESSING_HPP

#include <cstddef>
#include <vector>
#include <string>
#include "matching_rlz.hpp"

struct substitution {
    size_t pos;
    unsigned char ch;
};

struct insertion {
    size_t pos;
    unsigned char ch;
    bool operator < (const insertion& other) const
    {
        return (pos < other.pos);
    }
};

void make_substitutions(const char *ref, size_t n_ref, std::vector<struct match>& original_matchings,
                       std::vector<struct substitution>& new_substitutions);

void make_insertions(std::vector<struct match>& original_matchings,
                     std::vector<struct insertion>& new_insertions);

void make_delta_encoding(const std::string& fasta_line, size_t size_target,
                         const std::vector<struct match>& matchings, const std::vector<struct substitution>& subs,
                         const std::vector<struct insertion>& ins, char **ints_ptr, size_t& size_ints);

void make_delta_decoding(char *buf, size_t n, std::string& fasta_line, size_t& target_size, std::vector<struct match>& matchings,
                         std::vector<struct substitution>& subs, std::vector<struct insertion>& ins);

void multi_encode(const std::string& fasta_line, size_t size_target, const std::vector<struct match>& matchings,
                  const std::vector<struct substitution>& subs, const std::vector<struct insertion>& ins, char **encoded, size_t *size);

void multi_decode(char *buf, std::string& fasta_line, size_t& target_size, std::vector<struct match>& matchings,
                  std::vector<struct substitution>& subs, std::vector<struct insertion>& ins);


#endif //HIERARCHICALRELATIVELEMPELZIV_IDOCOMPMAXPOSTPROCESSING_HPP
