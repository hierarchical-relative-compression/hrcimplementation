//
// Created by mpelo on 2/3/25.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HRLZTESTS_H
#define HIERARCHICALRELATIVELEMPELZIV_HRLZTESTS_H

#include <stdint.h>
#include <string>

extern "C" {
    int compress_decompress_arith(char *str, uint64_t n);

    int64_t test_compress_hirgcmax(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta, uint64_t random);

    int64_t test_idocomp_compress_decompress(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

    int64_t test_rlz_compress_decompress(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

//    int64_t test_compress_sccg(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

    char *test_idocomp_parse_rlz(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

    char *test_idocompmax_parse_subs(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

    char *test_idocompmax_insertions_decomp(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta);

    void free_ptr(char *pointer);

    char *test_partial_sums(char *json, uint64_t n_json);

};

char *make_C_string(const std::string& str);

char *json_return_C(const std::string& message);

#endif //HIERARCHICALRELATIVELEMPELZIV_HRLZTESTS_H
