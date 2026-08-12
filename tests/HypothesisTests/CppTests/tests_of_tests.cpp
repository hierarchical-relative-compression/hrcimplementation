//
// Created by mpelo on 2/3/25.
//
#include <iostream>
#include <random>
#include <test_idocomp_max.hpp>

#include "test_compress_hirgc_max.hpp"
#include "test_idocomp_max.hpp"
#include "test_finitestateentropy.hpp"

int test_refs();

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    std::string ref_string {">0\nAAAAAAAAAAAAAAAAAAAA"};
    std::string tar_string {">0\nAAAAAAAAAAAAAAAAAAAA"};
    int ret = test_idocompmax_insertions_decomp(ref_string, tar_string);
    std::cerr << "compress_decompress returned " << ret << std::endl;
    return 0;
}
/*
 *
#include "../../../Legacy/tests/HypothesisTests/CppTests/test_compress_idocomp.hpp"
int test_refs() {

    std::filesystem::path ref_path = "/tmp/ref1.fasta";
    std::filesystem::path target_path = "/tmp/target1.fasta";
    uint64_t random = 3;
    try {
        hirgc_mrmcmax::HiRGCRef ref_obj(ref_path, random);
        hirgc_mrmcmax::HiRGCTarget *target = ref_obj.compress_target(target_path);
        hirgc_mrmcmax_old::HiRGCRef ref_old_obj(ref_path);
        hirgc_mrmcmax_old::HiRGCTarget *target_old = ref_old_obj.compress_target(target_path);
        auto pair = std::mismatch(target->matches.begin(), target->matches.end(), target_old->matches.begin(),
                                  [&](hirgc_mrmcmax::POSITION_RANGE m1, hirgc_mrmcmax_old::POSITION_RANGE m2) {
                                      if (m1.length != m2.length) return false;
                                      if (m1.begin == m2.begin)
                                          return true;
                                      size_t i_new = m1.begin;
                                      size_t i_old = m2.begin;
                                      size_t end = i_new + m1.length;
                                      size_t ix;
                                      for (; i_new < end; i_old++, i_new++) {
                                          ix = hirgc_mrmcmax::index(ref_old_obj.seq_ref[i_old]);
                                          if (ix < 4) {
                                              if (ref_obj.seq_ref[i_new] != ix)
                                                  return false;
                                          } else if (ref_obj.seq_ref[i_new] != ref_old_obj.seq_ref[i_old]) {
                                              return false;
                                          }
                                      }
                                      return true;
                                  });
        if (pair.first != target->matches.end() || pair.second != target_old->matches.end()) {
            std::cerr << "Difference in matches!!! New match: (" << pair.first->begin << ", " << pair.first->length << "), Old: ("
                      << pair.second->begin << ", " << pair.second->length << ")" << std::endl;
            std::cerr << "Error" << std::endl;
            return -1;
        }
        auto pair2 = std::mismatch(target->mismatched_sequences.begin(), target->mismatched_sequences.end(), target_old->mismatched_sequences.begin(),
            [&](const std::string& new_str, const std::string& old_str) {
                size_t size = new_str.size();
                if (size != old_str.size()) return false;
                for (size_t i = 0; i < size; i++) {
                    size_t ix = hirgc_mrmcmax::index(old_str[i]);
                    if (ix < 4) {
                        if (new_str[i] != (ix + '0')) return false;
                    } else if (new_str[i] != old_str[i]) return false;
                }
                return true;
            });
        if (pair2.first != target->mismatched_sequences.end() || pair2.second != target_old->mismatched_sequences.end()) {
            std::cerr << "Difference in mismatched sequences!!! New match: " << *pair2.first << ", Old: " << *pair2.second << std::endl;
            std::cerr << "Error" << std::endl;
            return -1;
        }
        return 0;
    } catch (std::ios_base::failure &e) {
        std::cerr << "IOError while compressing with HiRGC" << std::endl;
        std::cerr << e.what() << std::endl;
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        return -1;
    }
}*/

/**
*
    size_t seed = 3;
    std::ranlux48_base gen { seed };
    std::cout << gen() << std::endl;
    std::cout << gen() << std::endl;
    std::cout << gen() << std::endl;
*/
/*
std::string ar { "lolollollo" };
std::ranlux48_base r{ std::random_device{}() };
std::cout << r.max() << std::endl;
return compress_decompress_arith(ar);

std::string ref_str {">0\nACAAAAAAAAAAAAAAAAAA"};
std::string target_str {">0\nAAAAAAAAAAAAAAAAAAAA"};
int ret = test_idocompmax_insertions_decomp(ref_str, target_str);
return ret;
*/