//
// Created by mpelo on 6/11/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_HIRGC_MAX_HPP
#define HIERARCHICALRELATIVELEMPELZIV_HIRGC_MAX_HPP

/**
 * Re-implementation of HiRGC.
 * It is meant to fix the memory usage and performance issues
 * of the original HiRGC implementation.
 * We support for now only the compression of pre-processed files
 * (without line breaks in the FASTA, except the first one that separates
 * the FASTA definition file from the sequence).
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include "HiRGC32bitLinearProbing.hpp"
#include "AggregatingMultimap.hpp"
#include "StrongUniversalHashing.hpp"
#include "TabulationHashing.hpp"
#include "RefDataStructure.hpp"

namespace hirgc_mrmcmax {

    typedef uint32_t datatype;
    const auto max_size = UINT32_MAX;

    const std::size_t k = 20;

    struct POSITION_RANGE { //an interval
        datatype begin, length;//
    };

    struct POSITION_OTHER_CHAR {//for other characters
        datatype pos;
        uint8_t ch;
        //Note how the character is stored as an integer (of 8 bits). This will make the print to ASCII write the
        //code itself instead of the character, which is helpful in case weird characters appear.
    };

    uint64_t index(char c);

    struct HiRGCTarget {
        datatype file_length {};
        datatype sequence_length {};
        std::string fasta_line;
        //We don't store the length of the lines because we assume that
        //we have pre-processed the file to avoid line breaks
        std::vector<POSITION_RANGE> lowercase_intervals;
        std::vector<POSITION_RANGE> n_letter_intervals;
        std::vector<POSITION_OTHER_CHAR> other_chars;
        bool match_goes_first {};
        std::vector<POSITION_RANGE> matches;
        std::vector<std::string> mismatched_sequences;
    };

    void print_other_chars(std::ostream& out, POSITION_OTHER_CHAR *other_chars, std::size_t n_other_chars);
    void read_other_chars(std::istream& file, HiRGCTarget *target);
    std::string get_filename(const std::filesystem::path& ref_path, const std::filesystem::path& file_to_parse);

    /**
     * This class is meant to store a processed reference FASTA file
     */
    class HiRGCRef : public RefDataStructure {
    public:
        std::string id;
        uint64_t random;
        HiRGCRef(const std::filesystem::path& ref, bool decomp, uint64_t random = 0);
        HiRGCRef(std::istream& in, bool decomp, uint64_t random = 0);
        ~HiRGCRef();
        std::string get_filename(const std::filesystem::path& target_full_path) const;
        void parseRef(std::istream& in) override;
        void parseRefDecomp(std::istream& in);
        void compress_target(const std::filesystem::path& in, std::ostream& out) const override;
        void compress_target_to_disk(const std::filesystem::path& target_full_path, const std::filesystem::path& output_dir, bool print_stats);
        void compress_chromosome_to_disk(const std::filesystem::path& target_chr_full_path, const std::string& comp_target_full_name, const std::filesystem::path& full_archive, bool print_stats) const;
        HiRGCTarget *compress_target(const std::filesystem::path &target_path) const;
        void decompress_target(const std::filesystem::path& target_full_path, const std::filesystem::path& output_dir, const std::string& target_original_filename) const;
        void decompress_target(std::istream& in, std::ostream& out) const override;
        void decode_target(std::iostream& file, const std::filesystem::path& output_dir, const std::string& target_original_filename) const;
        std::string seq_ref;
        //Hash table: (k-mer, pos)
        HiRGC32bitLinearProbing match_hash_table;
        //UniversalMultiplyShift64 match_hash_table_func;
        //TabulationHashing40 match_hash_table_func {};
        StrongUniversalHashing64 match_hash_table_func { 32 };

        static std::string get_id_ref(const std::filesystem::path& ref);
    private:
        void parse_target(HiRGCTarget *target, std::unique_ptr<char[]>& seq, datatype n) const;
        void find_matches(HiRGCTarget *target, datatype seq_count, char *seq_DNA) const;
        void find_match(datatype *entries_pos, char *seq_DNA, datatype seq_count, datatype target_i, datatype& best_match, datatype& best_pos) const;
        //Experimental
        datatype find_match_fast(const char *seq_ref, const char *seq_tar, datatype seq_i, datatype tar_i, datatype seq_len, datatype tar_len) const;
        void serialize_target(HiRGCTarget *target, std::ostream& out, bool print_stats) const;
        void encode_adaptive(std::iostream& out, size_t n, const std::filesystem::path& output_file) const;
        void zip_target(const std::filesystem::path& file, const std::filesystem::path& archive_zip) const;
        void fse_encode_target(const std::filesystem::path& file, const std::filesystem::path& output_file) const;
        void deserialize(HiRGCTarget *target, std::unique_ptr<char[]> seq_DNA, std::ostream& out) const;
    };

}

#endif //HIERARCHICALRELATIVELEMPELZIV_HIRGC_MAX_HPP
