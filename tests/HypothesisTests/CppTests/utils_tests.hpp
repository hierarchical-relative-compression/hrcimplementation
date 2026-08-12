//
// Created by mpelo on 7/22/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H
#define HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H

#include <sstream>
#include <filesystem>
#include "Compressors.hpp"

/**
 * Returns the first position of a differing byte of both FASTA streams,
 * or -1 if they are equal.
 * @param stream1 First FASTA
 * @param stream2 Second FASTA
 * @return the position where they differ, or -1 otherwise.
 */
int64_t compare_fasta(std::istream& stream1, std::istream& stream2);

int serialize_two(const std::string& ref, const std::string& target,
                  std::filesystem::path& ref_path, std::filesystem::path& target_path);

/**
 * Makes the suffix array of the given FASTA file.
 * @param fasta path to the FASTA file
 * @return the error code of divsufsort (0 if successful, non-zero otherwise)
 */
int make_SA(const std::filesystem::path& fasta);

/**
 * Compresses and decompresses the target w.r.t. the reference using the given compressor.
 * Checks the decompressed output against the original target_fasta, and returns the index where they differ,
 * or -1 if they are the same (thus -1 is a successful return).
 * @param ref_fasta the reference string
 * @param target_fasta the target string to compress
 * @param SA whether the compressor uses a suffix array or not for the reference
 * @param comp the compressor
 * @return -1 if the decompressed string and @target_fasta are the same, or the index of the sequence where they differ.
 */
int64_t test_compress_decompress(const std::string& ref_fasta, const std::string& target_fasta, bool SA,
                                 std::unique_ptr<Compressors::Compressor> comp);

int
sufcheck(const unsigned char *T, const unsigned int *SA, unsigned int n, int verbose);

#endif //HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H
