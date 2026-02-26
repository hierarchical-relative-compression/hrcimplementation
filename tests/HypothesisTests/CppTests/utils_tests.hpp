//
// Created by mpelo on 7/22/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H
#define HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H

#include <sstream>
#include <filesystem>

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

int
sufcheck(const unsigned char *T, const unsigned int *SA, unsigned int n, int verbose);

#endif //HIERARCHICALRELATIVELEMPELZIV_COMPARE_FASTA_STREAMS_H
