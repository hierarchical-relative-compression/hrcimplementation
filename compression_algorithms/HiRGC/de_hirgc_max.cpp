//
// Created by mpelo on 6/13/24.
//

#include "include/hirgc/hirgc_max.hpp"
#include "AdaptiveScalingCoder.hpp"
#include <cstring>
#include <iostream>
#include <cmath>
#include "common/SteadyTimer.hpp"
#include <omp.h>

namespace hirgc_mrmcmax {

    void unzip_target(const std::filesystem::path& target, const std::filesystem::path& output_dir);
    void huff_decode_target(const std::filesystem::path& target, const std::filesystem::path& output_file);
    void rm_target_unzip_txt(const std::filesystem::path& target);

    char to_big_char(char bin) {
        switch (bin) {
            case '0':
                return 'A';
            case '1':
                return 'C';
            case '2':
                return 'G';
            case '3':
                return 'T';
            default:
                return 'N';
        }
    }

    /*
     * The file format is:
     * original_file_size
     * sequence_size
     * fasta_line
     * n_lowercase_intervals [pos length]*
     * n_n_intervals [pos length]*
     * n_other_chars [pos char_ix]*
     * n_matches n_sequences
     * match_goes_first ? 1 : 0
     * [pos length]
     * ACG
     * [pos length]
     * ...
     */
    void HiRGCRef::decompress_target(std::istream& file, std::ostream& out) const {
        //Read FASTA definition line, if it is there
        file.seekg(0);
        std::string line1, line2;
        HiRGCTarget *target = new HiRGCTarget();
        std::getline(file, line1);
        std::getline(file, line2);
        std::getline(file, target->fasta_line);
        size_t file_l = std::stoll(line1);
        if (file_l > max_size)
            throw std::runtime_error("Can't decompress: file size is too big for the allowed precision!  Please change the datatype and the max value in hirgc_max.hpp");
        target->file_length = static_cast<datatype>(file_l); //at most a 64-bit number
        target->sequence_length = static_cast<datatype>(std::stoll(line2));
        datatype n_lowercase_intervals, n_n_intervals;
        target->lowercase_intervals = std::vector<POSITION_RANGE>();
        target->n_letter_intervals = std::vector<POSITION_RANGE>();
        target->other_chars = std::vector<POSITION_OTHER_CHAR>();
        std::unique_ptr<char[]> seq_DNA = std::make_unique<char[]>(target->sequence_length);
        //Read lowercase intervals
        file >> n_lowercase_intervals;
        if (n_lowercase_intervals > 0) {
            target->lowercase_intervals.reserve(n_lowercase_intervals);
            datatype pos, length;
            for (datatype i = 0; i < n_lowercase_intervals; i++) {
                file >> pos >> length;
                target->lowercase_intervals.emplace_back(pos, length);
            }
        }
        //Read N intervals
        file >> n_n_intervals;
        if (n_n_intervals > 0) {
            target->n_letter_intervals.reserve(n_n_intervals);
            datatype pos, length;
            for (datatype i = 0; i < n_n_intervals; i++) {
                file >> pos >> length;
                target->n_letter_intervals.emplace_back(pos, length);
            }
        }
        //Read other chars
        /*
        file >> n_other_chars;
        if (n_other_chars > 0) {
            target->other_chars.reserve(n_other_chars);
            std::size_t pos;
            uint8_t ch_ix;
            for (std::size_t i = 0; i < n_other_chars; i++) {
                file >> pos >> ch_ix;
                target->other_chars.emplace_back(pos, ch_ix);
            }
        }
         */
        read_other_chars(file, target);
        datatype n_matches, n_sequences;
        int match_goes_first;
        file >> n_matches >> n_sequences >> match_goes_first;
        target->matches = std::vector<POSITION_RANGE>();
        target->matches.reserve(n_matches);
        target->mismatched_sequences = std::vector<std::string>();
        target->mismatched_sequences.reserve(n_sequences);
        //Read matches and decode at the same time
        datatype target_ix = 0;
        datatype prev_pos = 0;
        char *buf = seq_DNA.get();
        if (match_goes_first == 1) {
            datatype smallest = std::min(n_matches, n_sequences);
            std::string sequence;
            int64_t pos;
            datatype length;
            for (datatype i = 0; i < smallest; i++) {
                //First read match, then sequence
                file >> pos >> length >> sequence;
                //Delta decoding
                length += k;
                pos += prev_pos;
                prev_pos = pos + length;
                std::memcpy(&buf[target_ix], &seq_ref[pos], length);
                size_t end = target_ix + length;
                for (size_t i = target_ix; i < end; i++)
                    buf[i] = to_big_char(buf[i]+'0');
                target_ix += length;
                for (char c : sequence) {
                    buf[target_ix++] = to_big_char(c);
                }
            }
            if (n_matches > smallest) {
                //Last match
                file >> pos >> length;
                length += k;
                pos += prev_pos;
                std::memcpy(&buf[target_ix], &seq_ref[pos], length);
                size_t end = target_ix + length;
                for (size_t i = target_ix; i < end; i++)
                    buf[i] = to_big_char(buf[i]+'0');
            }
        } else {
            datatype smallest = std::min(n_matches, n_sequences);
            std::string sequence;
            int64_t pos;
            datatype length;
            datatype prev_pos = 0;
            for (datatype i = 0; i < smallest; i++) {
                //First read sequence, then match
                file >> sequence >> pos >> length;
                //Delta decoding
                length += k;
                pos += prev_pos;
                prev_pos = pos + length;
                for (char c : sequence) {
                    buf[target_ix++] = to_big_char(c);
                }
                std::memcpy(&buf[target_ix], &seq_ref[pos], length);
                size_t end = target_ix + length;
                for (size_t i = target_ix; i < end; i++)
                    buf[i] = to_big_char(buf[i]+'0');
                target_ix += length;
            }
            if (n_sequences > smallest) {
                //Last sequence
                file >> sequence;
                for (char c : sequence) {
                    buf[target_ix++] = to_big_char(c);
                }
            }
        }
        //Now, deserialize
        deserialize(target, std::move(seq_DNA), out);
        /*
        rm_target_unzip_txt(target_no_ext);
         */
        delete target;
    }

    void HiRGCRef::deserialize(HiRGCTarget *target, std::unique_ptr<char[]> seq_DNA, std::ostream& out) const {
        if (target->fasta_line != "") {
            out << target->fasta_line << "\n";
        }
        datatype seq_DNA_i = 0;
        datatype target_i = 0;
        datatype n = target->sequence_length;
        bool lc_flag = false, n_flag = false;
        //We are going to output the characters one by one, checking when we have to lowercase them or add a special
        //character
        auto lc_intervals_it = target->lowercase_intervals.begin();
        auto lc_intervals_end = target->lowercase_intervals.end();
        auto n_intervals_it = target->n_letter_intervals.begin();
        auto n_intervals_end = target->n_letter_intervals.end();
        auto other_chars_it = target->other_chars.begin();
        auto other_chars_end = target->other_chars.end();
        while (target_i < n) {
            //Check for lowercase
            if (lc_intervals_it != lc_intervals_end) {
                if (!lc_flag && lc_intervals_it->begin == target_i) {
                    lc_flag = true;
                } else if (lc_flag && target_i == lc_intervals_it->begin + lc_intervals_it->length) {
                    lc_flag = false;
                    lc_intervals_it++;
                }
            }
            //Check for N character
            if (n_intervals_it != n_intervals_end) {
                if (target_i == n_intervals_it->begin) {
                    n_flag = true;
                } else if (n_flag && target_i == n_intervals_it->begin + n_intervals_it->length) {
                    n_flag = false;
                    n_intervals_it++;
                }
            }
            //Print special characters if needed
            if (n_flag) {
                out << (lc_flag ? 'n' : 'N');
            } else if (other_chars_it != other_chars_end && target_i == other_chars_it->pos) {
                char c = other_chars_it->ch; //Note the type conversion
                out << (lc_flag ? static_cast<char>(tolower(c)) : static_cast<char>(c));
                other_chars_it++;
            } else {
                //Print normal character
                char c = seq_DNA[seq_DNA_i++];
                out << (lc_flag ? static_cast<char>(tolower(c)) : static_cast<char>(c));
            }
            target_i++;
        }
    }

    void unzip_target(const std::filesystem::path& target, const std::filesystem::path& output_dir) {
        std::string cmd = "7za x \"";
        cmd.append(target.string()).append("\" -o\"").append(std::filesystem::canonical(output_dir).string()).append("\"").append(" >/dev/null");
#ifndef NDEBUG
        std::cout << "Calling 7za with " << cmd << std::endl;
#endif
        int ret = system(cmd.c_str());
    }

    void huff_decode_target(const std::filesystem::path& target, const std::filesystem::path& output_file) {
        std::string cmd = DIR_FSE;
        cmd.append("/fse_exec -d \"").append(std::filesystem::canonical(target)).append("\" \"")
            .append(output_file.string()).append("\" >/dev/null 2>/dev/null");
        int ret = system(cmd.c_str());
        if (ret != 0) {
            std::cerr << "WARNING: Call to Huffman encode returned exit code " << ret << std::endl;
        }
    }

    void rm_target_unzip_txt(const std::filesystem::path& target) {
        std::string cmd = "rm \"";
        cmd.append(target.c_str()).append("\"");
#ifndef DEBUG
        std::cout << "Calling rm with " << cmd << std::endl;
#endif
        int ret = system(cmd.c_str());
    }

    /**
     * Format for other chars:
     * n_other_chars pos1 pos2 .. pos_n
     * n_char_types char1 char2 .. char_m
     * char_type_1 char_type_2 .. char_type_n
     *
     * If there are less than 10 character types, the type of each character is specified as a decimal digit.
     * If there are 10 or more, it is specified as an index into the char_types array. These indices are
     * packed into 32-bit words.
     */
    void read_other_chars(std::istream& file, HiRGCTarget *target) {
        std::size_t n_other_chars = 0;
        file >> n_other_chars;
        if (n_other_chars > 0) {
            target->other_chars.reserve(n_other_chars);
            std::size_t pos;
            for (std::size_t i = 0; i < n_other_chars; i++) {
                file >> pos;
                target->other_chars.emplace_back(pos, 'A');
            }
            //Read the type of chars
            std::size_t n_type_chars;
            file >> n_type_chars;
            std::vector<int> char_types(n_type_chars);
            for (uint i = 0; i < n_type_chars; i++) {
                file >> char_types[i];
            }
            //Read the sequence of indices on the array of char types
            if (n_type_chars < 10) {
                //Read digits one by one
                char d;
                for (std::size_t i = 0; i < n_other_chars; i++) {
                    file >> d;
                    d -= '0'; //To make it an index
                    target->other_chars[i].ch += char_types[d];
                }
            } else {
                //Read packed ints
                uint bits_to_represent_char = std::ceil(std::log2(n_type_chars));
                std::size_t chars_in_int = std::floor(32.0 / bits_to_represent_char);
                std::size_t n_ints = std::ceil(static_cast<float>(n_other_chars) / static_cast<float>(chars_in_int));
                //Get a mask to extract only the lower bits of each int
                uint32_t mask = (1 << bits_to_represent_char) - 1; //Also works for 32-bit
                for (std::size_t i = 0; i < n_ints; i++) {
                    uint32_t n;
                    file >> n;
                    //Minimum between the maximum number of chars in an int, and the number of
                    //remaining chars to read
                    for (int j = std::min(chars_in_int - 1, n_other_chars - i*chars_in_int - 1); j >= 0; j--) {
                        uint32_t masked = n & mask;
                        target->other_chars[i*chars_in_int + j].ch += char_types[masked];
                        n >>= bits_to_represent_char;
                    }
                }
            }
        }
    }
}