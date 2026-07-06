//
// Created by mpelo on 6/11/24.
//

/**
 * Re-implementation of HiRGC.
 * It is meant to fix the memory usage and performance issues
 * of the original HiRGC implementation.
 * We support for now only the compression of pre-processed files
 * (without line breaks in the FASTA, except the first one that separates
 * the FASTA definition file from the sequence).
 */

#include <iostream>
#include "include/hirgc/hirgc_max.hpp"
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include "utils/PrettyPrint.hpp"
#include <chrono>
#include <bit>
#include "RefDataStructure.hpp"

namespace hirgc_mrmcmax {

    std::size_t count_factors(HiRGCTarget *target);

    void rm_target_txt(const std::filesystem::path& target);

    void print_compression_stats(size_t n_matches, size_t n_mismatched_seqs, size_t total_size_mismatched_seqs,
                                 size_t n_lowercase, size_t n_n_chars, size_t n_other_chars, size_t total_size);

    std::string get_filename(const std::filesystem::path& ref_path, const std::filesystem::path& file_to_parse) {
        std::string new_filename = file_to_parse.filename().string().append("_ref_").append(ref_path.filename());
        return new_filename;
    }

    std::string HiRGCRef::get_filename(const std::filesystem::path& target_full_path) const {
        std::string new_filename = target_full_path.filename().string().append("_ref_").append(id);
        return new_filename;
    }

    uint64_t index(char c) {
        switch (c) {
            case 'A':
                return 0;
            case 'C':
                return 1;
            case 'G':
                return 2;
            case 'T':
                return 3;
            default:
                return 4;
        }
    }

    std::size_t hash_function(std::size_t x, std::size_t table_len) {
        return x & (uint64_t)((1 << 30) - 1);
    }

    HiRGCRef::HiRGCRef(const std::filesystem::path& ref, bool decomp, uint64_t rand) : RefDataStructure(ref)
        , id(get_id_ref(ref)), random(rand) {
        //Note how we have text streams for the outputted compressed target
        out_tar_mode = std::ios_base::out;
        in_decomp_tar_mode = std::ios_base::in;
        //Open file
        std::ifstream file(ref, in_ref_mode);
        if (!file) {
            std::cerr << "File not found or empty: " << ref << std::endl;
            std::exit(1);
        }
        if (decomp) {
            parseRefDecomp(file);
        } else {
            parseRef(file);
        }
    }

    HiRGCRef::HiRGCRef(std::istream& in, bool decomp, uint64_t rand) : RefDataStructure(in), id("ref")
        , random(rand) {
        //Note how we have text streams for the outputted compressed target
        out_tar_mode = std::ios_base::out;
        in_decomp_tar_mode = std::ios_base::in;
        if (decomp) {
            parseRefDecomp(in);
        } else {
            parseRef(in);
        }
    }

    HiRGCRef::~HiRGCRef() {}

    std::string HiRGCRef::get_id_ref(const std::filesystem::path& ref) {
        std::string ret;
        ret = ref.filename().string();
        return ret;
    }


    /**
     * Processes a reference FASTA in the stream.
     * @file input stream containing the FASTA file
     */
    void HiRGCRef::parseRef(std::istream& file) {
        //Get file size
        file.seekg(0, file.end);
        size_t length_file = file.tellg();
        file.seekg(0, file.beg);
        if (length_file > static_cast<size_t>(max_size)) {
            throw std::runtime_error("The file size of the reference is greater than the allowed precision!!! Please change the datatype and the max value in hirgc_max.hpp");
        }
        datatype length = static_cast<datatype>(length_file);
        //If the first line is FASTA definition, ignore it
        char c = file.peek();
        if (c == '>') {
            std::string fasta;
            std::getline(file, fasta);
            //Reduce the length of the data
            datatype curr_pos = static_cast<datatype>(file.tellg());
            length = length - curr_pos;
        }
        if (length == 0) {
            std::cerr << "Empty file in HiRGCRef!!! Exiting" << std::endl;
            std::exit(1);
        }
        auto t1 = std::chrono::steady_clock::now();
        //Allocate memory to read
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(length);
        file.read(buf.get(), length);
        //Allocate the data structures. We might allocate a bit more space than necessary
        //if there are other characters than A, C, G and T.
        seq_ref = std::string();
        seq_ref.reserve(length);
        datatype max_n_tuples = std::max(0ul, length - k + 1);
        size_t cap = std::bit_ceil(static_cast<size_t>(max_n_tuples) << 1);
        int range = std::bit_width(cap) - 1;
        if (random == 0) {
            match_hash_table_func = StrongUniversalHashing64(range);
            //match_hash_table_func = TabulationHashing40();
        } else {
            match_hash_table_func = StrongUniversalHashing64(random, range);
            //match_hash_table_func = TabulationHashing40(random);
            //std::cout << "Random seed: " << random << std::endl;
        }
        //std::cout << "Building hash table..." << std::endl;
        //Hash table for the last element in the chain and the entries
        match_hash_table = HiRGC32bitLinearProbing(max_n_tuples, &match_hash_table_func);
        //Iterate, ignore everything that is not A, C, G, T,
        //and make the tuples.
        //First tuple
        datatype i = 0;
        size_t current_tuple = 0;
        datatype tuple_i = 0;
        size_t l = 0;
        while (l < k && i < length) {
            c = buf[i++];
            c = toupper(c);
            uint64_t ix = index(c);
            if (ix > 3) {
                //Ignore, bad character
                continue;
            }
            seq_ref += ix;
            current_tuple += ix << (2*l);
            l++;
        }
        if (l == k) {
            //Add the first tuple
            match_hash_table.insert_copy(current_tuple, tuple_i);
            tuple_i++;
        }
        //Continue adding the rest of tuples
        size_t shift_last_character = 2*(k-1);
        while (i < length) {
            char c = buf[i++]; //Always advance i counter
            c = toupper(c); //In case there are lowercase letters
            uint64_t ix = index(c);
            if (ix > 3) {
                //Bad character, ignore (and don't advance j counter)
                continue;
            }
            seq_ref += ix;
            //We have a good character, create the next tuple from the previous one
            //By shifting out the least significant character and adding the next one at front
            current_tuple >>= 2;
            current_tuple += (ix << shift_last_character);
            //Update hash table
            match_hash_table.insert_copy(current_tuple, tuple_i);
            tuple_i++;
        }
        //rearrange
        //std::cout << "Rearranging..." << std::endl;
        match_hash_table.rearrange();
        auto t2 = std::chrono::steady_clock::now();
        auto dur = t2 - t1;
        //std::cout << "Ref parsed. Time elapsed: " << format_time(dur.count()) << std::flush;
    }
    //Parse and process the reference without building the hash table
    void HiRGCRef::parseRefDecomp(std::istream& file) {
        file.seekg(0, file.end);
        size_t length_file = file.tellg();
        file.seekg(0, file.beg);
        if (length_file > static_cast<size_t>(max_size)) {
            throw std::runtime_error("The file size of the reference is greater than the allowed precision!!! Please change the datatype and the max value in hirgc_max.hpp");
        }
        datatype length = static_cast<datatype>(length_file);
        char c = file.peek();
        if (c == '>') {
            std::string fasta;
            std::getline(file, fasta);
            datatype curr_pos = static_cast<datatype>(file.tellg());
            length = length - curr_pos;
        }
        if (length == 0) {
            std::cerr << "Empty file in HiRGCRef!!! Exiting" << std::endl;
            std::exit(1);
        }
        //Allocate memory to read
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(length);
        file.read(buf.get(), length);
        //make the cleaned reference
        seq_ref = std::string();
        seq_ref.reserve(length);
        datatype max_n_tuples = std::max(0ul, length - k + 1);
        datatype i = 0;
        size_t l = 0;
        while (l < k && i < length) {
            c = buf[i++];
            c = toupper(c);
            uint64_t ix = index(c);
            if (ix > 3)
                continue; //Bad character
            seq_ref += ix;
            l++;
        }
        while (i < length) {
            c = buf[i++];
            c = toupper(c);
            uint64_t ix = index(c);
            if (ix > 3)
                continue; //Bad character
            seq_ref += ix;
        }
    }

    void HiRGCRef::compress_target(const std::filesystem::path& in, std::ostream &out) const {
        HiRGCTarget *target_compressed = compress_target(in);
        serialize_target(target_compressed, out, false);
        delete target_compressed;
    }

    HiRGCTarget *HiRGCRef::compress_target(const std::filesystem::path &target_path) const {
        //std::cout << "Compressing target " << target_path << " with ref " << id << std::endl;
        //Target data:
        HiRGCTarget *target = new HiRGCTarget();
        //Read FASTA definition line, if it is there
        std::ifstream file;
        file.exceptions(std::ifstream::badbit);
        file.open(target_path, in_tar_mode);
        if (!file) {
            std::cerr << "File not found or emtpy when compressing target: " << target_path << std::endl;
            std::exit(1);
        }
        //Get file size
        file.seekg(0, file.end);
        size_t file_l = file.tellg();
        if (file_l > static_cast<size_t>(max_size)) {
            delete target;
            throw std::runtime_error("The file size of the target is greater than the allowed precision!!! Please change the datatype and the max value in hirgc_max.hpp");
        }
        target->file_length = static_cast<datatype>(file_l);
        file.seekg(0, file.beg);
        datatype sequence_length = target->file_length;
        char c = file.peek();
        if (c == '>') {
            std::getline(file, target->fasta_line);
            //Reduce the length of the sequence data
            datatype curr_pos = static_cast<datatype>(file.tellg());
            sequence_length = sequence_length - curr_pos;
        } else {
            target->fasta_line = "";
        }
        target->sequence_length = sequence_length;
        //Allocate memory to read
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(sequence_length);
        file.read(buf.get(), sequence_length);
        file.close();
        //We have read the target file into memory. Now parse it
        parse_target(target, buf, sequence_length);
        return target;
    }

    void HiRGCRef::parse_target(HiRGCTarget *target, std::unique_ptr<char[]>& seq, datatype n) const {
        auto parse_target_t1 = std::chrono::steady_clock::now();
        target->lowercase_intervals = std::vector<POSITION_RANGE>();
        target->n_letter_intervals = std::vector<POSITION_RANGE>();
        target->other_chars = std::vector<POSITION_OTHER_CHAR>();
        char *seq_DNA = (char*) malloc(n+1);
        datatype seq_count = 0;
        char c;
        bool lowercase_flag = false, n_flag = false;
        int ix;
        datatype i = 0;
        //First, we will process the extra information. Then, we will find the matches
        while (i < n) {
            c = seq[i];
            //Handle lowercase intervals
            if (islower(c) && !lowercase_flag) {
                target->lowercase_intervals.emplace_back(i);
                lowercase_flag = true; //We are in a lowercase region
            }
            if (isupper(c) && lowercase_flag) {
                POSITION_RANGE& pos = target->lowercase_intervals.back();
                pos.length = i - pos.begin;
                lowercase_flag = false; //End of lowercase region
            }
            c = toupper(c);
            //Handle N intervals
            if (c == 'N' && !n_flag) {
                target->n_letter_intervals.emplace_back(i);
                n_flag = true; //Beginning of an N region
            }
            if (c != 'N' && n_flag) {
                POSITION_RANGE& pos = target->n_letter_intervals.back();
                pos.length = i - pos.begin;
                n_flag = false; //End of N region
            }
            ix = index(c);
            if (ix > 3 && c != 'N') {
                if (c == '\n') {
                    //std::cerr << "New line! Ignoring" << std::endl;
                    i++;
                    continue; //Ignore this character
                }
                //Other character, save if it is a valid character (otherwise stop).
                else if (c < 'A') {
                    std::cerr << "Bad character in the sequence: " << c << ", terminating" << std::endl;
                    throw std::runtime_error("Bad character");
                } else {
                    target->other_chars.emplace_back(i, c-'A');
                }
            } else if (ix <= 3) {
                //Regular character, save in the DNA sequence
                seq_DNA[seq_count++] = ix;
            } //Else N, nothing to do
            i++;
        }
        seq_DNA[seq_count] = '\0';
        if (lowercase_flag) {
            //Sequence ended in lowercase
            POSITION_RANGE& pos = target->lowercase_intervals.back();
            pos.length = i - pos.begin;
        }
        if (n_flag) {
            //Sequence ended with an N
            POSITION_RANGE& pos = target->n_letter_intervals.back();
            pos.length = i - pos.begin;
        }
        //We could delete seq here, since all the data has been processed now
        seq.reset();
        //Now, we process the matches.
        find_matches(target, seq_count, seq_DNA);
        //Done!
        free(seq_DNA);
        auto parse_target_t2 = std::chrono::steady_clock::now();
        auto dur = parse_target_t2 - parse_target_t1;
        //std::cout << "Parse target duration: " << format_time(dur.count()) << std::endl;
    }

    void HiRGCRef::find_matches(HiRGCTarget *target, datatype seq_count, char *seq_DNA) const {
        auto find_matches_t1 = std::chrono::steady_clock::now();
        target->matches = std::vector<POSITION_RANGE>();
        target->mismatched_sequences = std::vector<std::string>();
        //Upper bound the memory
        target->matches.reserve((seq_count / 2) + 2);
        target->mismatched_sequences.reserve((seq_count / 2) + 2);
        if (seq_count < k) {
            //There will be no matches, only an unmatched sequence
            target->match_goes_first = false;
            std::string dna(seq_DNA, seq_count);
            for (datatype i = 0; i < seq_count; i++)
                dna[i] += '0';
            target->mismatched_sequences.push_back(dna);
            return;
        }
        //Read first tuple
        size_t current_tuple = 0;
        for (size_t l = 0; l < k; l++) {
            current_tuple += static_cast<size_t>(seq_DNA[l]) << (l*2);
        }
        //See if the first sub-sequence will be matched or unmatched
        //std::size_t hash = hash_function(current_tuple, hash_table_length);
        datatype *pos = match_hash_table.find(current_tuple);
        if (pos != nullptr) {
            //Still have to verify that it is indeed a match and not a hash collision
            datatype match_len = 0;
            datatype i_char_ref = *pos++;
            bool found = false;
            while ((i_char_ref != max_size) && !found) {
                while (match_len < k && seq_DNA[match_len] == seq_ref[i_char_ref + match_len]) {
                    match_len++;
                }
                found = match_len == k;
                match_len = 0;
                i_char_ref = *pos++;
            }
            target->match_goes_first = found;
        } else {
            target->match_goes_first = false;
        }
        datatype target_i = 0; //Index on the target sequence
        //Matching loop: Find match. If there is match, add the last unmatched sequence and add the match. Then,
        //add the next character and continue. Otherwise, just add this character to the unmatched sequence and continue.
        std::string mismatched = std::string();
        double find_match_count = 0;
        do {
            //Condition to enter loop: the tuple has been computed, and the hash for this tuple has been computed
            //Find first match
            datatype *entries_pos = match_hash_table.find(current_tuple);
            if (entries_pos != nullptr) {
                //Get full match
                datatype best_match;
                datatype best_pos;
                //We know there's at least 1 match
                auto find_match_t1 = std::chrono::steady_clock::now();
                find_match(entries_pos, seq_DNA, seq_count, target_i, best_match, best_pos);
                auto dur = std::chrono::steady_clock::now() - find_match_t1;
                find_match_count += dur.count();
                //We have found the best match. But maybe the best match is of size <k
                //if there were hash collisions. In that case, we treat it as a mismatch.
                if (best_match < k) {
                    mismatched += seq_DNA[target_i++] + '0';
                    //The next tuple can be computed from the current tuple
                    if (target_i + k - 1 < seq_count) {
                        current_tuple >>= 2;
                        current_tuple += static_cast<size_t>(seq_DNA[target_i + k - 1]) << (2*(k-1));
                    }
                } else {
                    //Add the last mismatched sequence, and add the next character to
                    //the new mismatched sequence.
                    if (mismatched.size() > 0) { //Could be 0 on the first match
                        target->mismatched_sequences.push_back(std::move(mismatched));
                    }
                    target->matches.emplace_back(best_pos, best_match);
                    mismatched = std::string();
                    if (target_i + best_match < seq_count) {
                        mismatched += seq_DNA[target_i + best_match] + '0';
                        target_i += best_match + 1;
                    } else {
                        target_i += best_match; //End of sequence
                    }
                    if (target_i + k - 1 < seq_count) {
                        //Must compute the next tuple
                        current_tuple = 0;
                        for (size_t i = 0; i < k; i++) {
                            current_tuple += static_cast<size_t>(seq_DNA[target_i + i]) << (2 * i);
                        }
                    }
                }
            } else {
                mismatched += seq_DNA[target_i++] + '0';
                //The next tuple can be computed from the current tuple
                if (target_i + k - 1 < seq_count) {
                    current_tuple >>= 2;
                    current_tuple += static_cast<size_t>(seq_DNA[target_i + k - 1]) << (2*(k-1));
                }
            }
        } while (target_i < seq_count - k + 1);
        //Add last characters, if any
        while (target_i < seq_count) {
            mismatched += seq_DNA[target_i++] + '0';
        }
        if (mismatched.size() > 0) {
            target->mismatched_sequences.push_back(mismatched);
        }
        auto find_matches_t2 = std::chrono::steady_clock::now();
        auto dur = find_matches_t2 - find_matches_t1;
        //std::cout << "Find matches time: " << format_time(dur.count()) << std::endl;
        //std::cout << "Find match time: " << format_time(find_match_count) << std::endl;
    }


    void HiRGCRef::find_match(datatype *entries_pos, char *seq_DNA, datatype seq_count, datatype target_i, datatype& best_match, datatype& best_pos) const {
        best_pos = 0;
        best_match = 0;
        //datatype i_tar = target_i;
        datatype match_len = 0;
        datatype n = seq_ref.size();
        datatype i_char_ref = *entries_pos++;
        //datatype i_ref = i_char_ref;
        const char *seq_ref_char = seq_ref.c_str();
        do {    //We only call here if there is at least 1 position to explore
            //Find match
            //Experimental
            /*
            while (i_tar < seq_count && i_ref < n && seq_DNA[i_tar++] == seq_ref_char[i_ref++]) {
                match_len++;
            }
             */
            match_len = find_match_fast(seq_ref_char, seq_DNA, i_char_ref, target_i, n, seq_count);
            if (match_len > best_match) {
                best_match = match_len;
                best_pos = i_char_ref;
            }
            //We have a full match of size match_len, at the reference position i_char_ref.
            //Now, we have to see if there are better matches
            //match_len = 0;
            //i_tar = target_i;
            i_char_ref = *entries_pos++;
            //i_ref = i_char_ref;
        } while (i_char_ref != max_size);
    }

    /**
     * Returns the number of trailing bytes that are equal in seq_word and tar_word.
     */
    inline static datatype last_comparison(uint64_t seq_word, uint64_t tar_word) {
        uint64_t x = seq_word ^ tar_word; //Non-zero bytes are different bytes in seq_word and tar_word
        //Number of trailing zero bytes
        return (std::countr_zero(x) >> 3); //nº of trailing zero bits divided by 8 (floored)
    }

    datatype HiRGCRef::find_match_fast(const char *seq_ref, const char *seq_tar, datatype seq_i, datatype tar_i, datatype seq_len, datatype tar_len) const {
        datatype max_seq = seq_len - seq_i;
        datatype max_tar = tar_len - tar_i;
        datatype max_match = max_seq < max_tar ? max_seq : max_tar;
        datatype match_len = 0;
        const uint64_t *ref_word = reinterpret_cast<const uint64_t*>(&seq_ref[seq_i]);
        const uint64_t *tar_word = reinterpret_cast<const uint64_t*>(&seq_tar[tar_i]);
        datatype max_words = max_match >> 3; // floor(max_match / 8)
        for (; match_len < max_words && *ref_word == *tar_word; match_len++)
        {
            ref_word++;
            tar_word++;
        }
        if (match_len < max_words) {
            match_len = match_len << 3; //x8 (word size)
            //Last comp
            return match_len + last_comparison(*ref_word, *tar_word);
        } else {
            //Check the last bytes
            match_len = match_len << 3;
            seq_i += match_len;
            tar_i += match_len;
            while (seq_i < seq_len && tar_i < tar_len && seq_ref[seq_i++] == seq_tar[tar_i++]) {
                match_len++;
            }
            return match_len;
        }
    }

    void HiRGCRef::serialize_target(HiRGCTarget *target, std::ostream& out, bool print_stats) const {
        //std::cout << "Serializing..." << std::endl;
        //auto serialize_t1 = std::chrono::steady_clock::now();
        //First, original file size
        out << target->file_length << "\n";
        out << target->sequence_length << "\n";
        //We don't output the line breaks / line lengths
        out << target->fasta_line << "\n";
        //Lowercase intervals
        std::size_t n_lowercase_intervals = target->lowercase_intervals.size();
        out << n_lowercase_intervals << " ";
        if (n_lowercase_intervals > 0) {
            POSITION_RANGE *lc_intervals = target->lowercase_intervals.data();
            out << lc_intervals[0].begin << " " << lc_intervals[0].length;
            for (std::size_t i = 1; i < n_lowercase_intervals; i++) {
                out << " " << lc_intervals[i].begin << " " << lc_intervals[i].length;
            }
        }
        out << "\n";
        //N letter intervals
        std::size_t n_n_intervals_size = target->n_letter_intervals.size();
        out << n_n_intervals_size << " ";
        if (n_n_intervals_size > 0) {
            POSITION_RANGE *n_intervals = target->n_letter_intervals.data();
            out << n_intervals[0].begin << ' ' << n_intervals[0].length;
            for (std::size_t i = 1; i < n_n_intervals_size; i++) {
                out << " " << n_intervals[i].begin << ' ' << n_intervals[i].length;
            }
        }
        out << "\n";
        //Other characters
        POSITION_OTHER_CHAR *other_chars = target->other_chars.data();
        print_other_chars(out, other_chars, target->other_chars.size());
        out << "\n";

        //Now, the sequence itself
        std::size_t n_matches = target->matches.size();
        std::size_t n_sequences = target->mismatched_sequences.size();
        out << n_matches << ' ' << n_sequences << '\n';
        out << (target->match_goes_first ? 1 : 0) << "\n";
        POSITION_RANGE *matches = target->matches.data();
        //We apply "delta compression": store each position as the difference to the previous position plus the length of the match
        datatype prev_pos = 0;
        std::vector<char> signs(n_matches);
        for (size_t i = 0; i < n_matches; i++) {
            int64_t diff = static_cast<int64_t>(matches[i].begin) - prev_pos;
            prev_pos = matches[i].begin + matches[i].length;
            long abs_val = std::abs(diff);
            if (abs_val > max_size) {
                throw std::runtime_error("Precision error in serializing!!!");
            }
            matches[i].begin = abs_val;
            signs[i] = (diff < 0LL) ? '-' : '+';
        }
        //And the last optimization: we decrease length by k because every match has at least length k
        for (size_t i = 0; i < n_matches; i++) {
            matches[i].length -= k;
        }
        std::string *sequences = target->mismatched_sequences.data();
        if (target->match_goes_first) {
            datatype smallest = std::min(n_matches, n_sequences);
            for (datatype i = 0; i < smallest; i++) {
                out << signs[i] << matches[i].begin << ' ' << matches[i].length << '\n';
                out << sequences[i] << '\n';
            }
            //If we start with a match, maybe there is one more match than mismatched sequences at the end.
            //If it ends with a mismatched sequence, the number of matches is the same as the number of
            //mismatched sequences.
            if (n_matches > smallest) {
                //There's a last match to output
                out << signs[smallest] << matches[smallest].begin << ' ' << matches[smallest].length << '\n';
            }
        } else {
            datatype smallest = std::min(n_matches, n_sequences);
            for (datatype i = 0; i < smallest; i++) {
                out << sequences[i] << '\n';
                out << signs[i] << matches[i].begin << ' ' << matches[i].length << '\n';
            }
            //If we start with a mismatched sequence, maybe there is a last mismatched sequence at the end.
            //If it ends with a match, there is the same number of matches than mismatched sequences.
            if (n_sequences > smallest) {
                //There's a last sequence to output
                out << sequences[smallest] << '\n';
            }
        }
        /*
#ifndef NDEBUG
        //We are going to write to a file first
        new_filename += ".factors";
        std::ofstream filefactors(output_dir / new_filename, std::ios_base::binary);
        filefactors << out.rdbuf()->str();
        filefactors.close();
#endif
         */
        size_t n = out.tellp(); //Bytes written
        if (print_stats) {
            //Print stats of the serialized file
            //We need to compute the size of the mismatched sequences
            size_t size_mismatched_sequences = 0;
            for (size_t i = 0; i < n_sequences; i++) {
                size_mismatched_sequences += sequences[i].size();
            }
            print_compression_stats(n_matches, n_sequences, size_mismatched_sequences, n_lowercase_intervals,
                                    n_n_intervals_size, target->other_chars.size(), n);
        }
        //std::cout << "Serialize time: " << format_time(dur) << std::endl;
    }

    std::size_t count_factors(HiRGCTarget *target) {
        std::size_t n_factors = 0;
        n_factors += target->matches.size();
        for (const std::string& s : target->mismatched_sequences) {
            n_factors += s.size();
        }
        return n_factors;
    }

    void HiRGCRef::zip_target(const std::filesystem::path& file, const std::filesystem::path& archive_zip) const {
        std::ostringstream oss;
        //The canonical path is needed because of a weird bug: if the path was serialized as "folder/file.fa" instead of "./folder/file.fa",
        //then 7za would take "folder/file.fa" as the file name and create a subdirectory in the 7z file, thus messing up with the
        //folder structure later on.
        oss << DIR_7ZA << "/7za a \"" << archive_zip.string() << "\" \"" << std::filesystem::canonical(file).c_str() << "\" -m0=PPMd" << " >/dev/null";
        std::string cmd = oss.str();
        //std::cout << "Calling 7za with " << cmd << std::endl;
        int ret = system(cmd.c_str());
        if (ret != 0)
            throw std::runtime_error("7za returned bad value: " + std::to_string(ret));
        oss.clear();
        //Check that the zip file is there
        if (std::filesystem::exists(archive_zip)) {
            std::filesystem::remove(file);
        } else {
            std::cerr << "Zip file " << archive_zip << " does not exist" << std::endl;
            std::exit(1);
        }
    }

    /**
    * The output file will contain the extension .huf
    * @param file full path to the input file.
    * @param output_file full path to the output file, with extension .huf
    */
    void HiRGCRef::fse_encode_target(const std::filesystem::path& file, const std::filesystem::path& output_file) const {
        std::string cmd = DIR_FSE;
        cmd.append("/fse_exec -h \"").append(std::filesystem::canonical(file)).append("\" \"")
            .append(output_file.string()).append("\" >/dev/null 2>/dev/null");
        int ret = system(cmd.c_str());
        if (ret != 0) {
            std::cerr << "WARNING: Call to fse returned exit code " << ret << std::endl;
        }
        if (!std::filesystem::exists(output_file)) {
            throw std::runtime_error("Output of fse compression " + output_file.string() + " does not exist!!!");
        }
        std::filesystem::remove(file);
    }

    void rm_target_txt(const std::filesystem::path& target) {
        std::string cmd = "rm \"";
        cmd.append(target.c_str()).append("\"");
        std::cout << "Calling rm with " << cmd << std::endl;
        int ret = system(cmd.c_str());
    }

    void print_other_chars(std::ostream& out, POSITION_OTHER_CHAR *other_chars, std::size_t n_other_chars) {
        //There are 62 characters in the ASCII code from 'B' ('A' is a regular char) to code 127
        std::vector<int> i_char(62, -1);
        std::vector<uint8_t> char_types;
        std::size_t i;
        //Print the positions and index the different characters
        out << n_other_chars;
        if (n_other_chars == 0) return;
        for (i = 0; i < n_other_chars; i++) {
            out << " " << other_chars[i].pos;
            uint8_t ch = other_chars[i].ch;
            if (i_char[ch] == -1) {
                //New char
                i_char[ch] = char_types.size();
                char_types.push_back(ch);
            }
        }
        out << std::endl;
        //Print the different characters
        uint size = char_types.size();
        out << size;
        for (i = 0; i < size; i++) {
            out << " " << std::to_string(char_types[i]);
        }
        //Now print the indices. If there are less than 10 different ones, print the decimal digit corresponding to
        //each one, in text.
        out << "\n";
        if (size < 10) {
            for (i = 0; i < n_other_chars; i++) {
                out << std::to_string(i_char[other_chars[i].ch]);
            }
        } else {
            //Print them, packed in 32-bit integers
            uint bits_to_represent_char = std::ceil(std::log2(size));
            uint chars_in_int = std::floor(32.0 / bits_to_represent_char);
            //No i++ in the outer for because it is done in the inside for
            for (i = 0; i < n_other_chars;) {
                uint32_t v = 0;
                //Write chars_in_int characters in a 32-bit int
                for (uint j = 0; j < chars_in_int && i < n_other_chars; j++, i++) {
                    v <<= bits_to_represent_char;
                    v += i_char[other_chars[i].ch];
                }
                //Write v in text
                out << " " << v;
            }
        }
    }

    void print_compression_stats(size_t n_matches, size_t n_mismatched_seqs, size_t total_size_mismatched_seqs,
                                 size_t n_lowercase, size_t n_n_chars, size_t n_other_chars, size_t total_size) {
        std::cout << n_matches << ";" << n_mismatched_seqs << ";" << format_size(total_size_mismatched_seqs) << ";" << n_lowercase;
        std::cout << ";" << n_n_chars << ";" << n_other_chars << ";" << total_size << ";" << format_size(total_size) << "\n";
    }
}

/**
* LEGACY CODE THAT MIGHT BE USEFUL LATER
*/
/*
Zipping a file:

            zip_target(factors_file);
            //Check that the zip file is there
            std::string zip_filename = filename_id + ".7z";
            std::filesystem::path zip_file = output_dir / zip_filename;
            if (std::filesystem::exists(zip_file)) {
                rm_target_txt(output_dir / filename_id);
            } else {
                std::cerr << "Zip file " << zip_file << " does not exist" << std::endl;
            }


    std::size_t HiRGCRef::compress_target_to_factors(const std::filesystem::path& target_full_path) {
        HiRGCTarget *target_compressed = compress_target(target_full_path);
        //We have parsed the target. Now, count the number of factors and return
        std::size_t factors = count_factors(target_compressed);
        delete target_compressed;
        return factors;
    }

    void HiRGCRef::encode_adaptive(std::iostream& out, size_t n, const std::filesystem::path& out_path) const {
        //std::cout << "Finished serializing, adaptive compression now..." << std::endl;
        //auto t_ac = std::chrono::steady_clock::now();
        AdaptiveScalingCoder ac = AdaptiveScalingCoder(false);
        ac.encode(out, n, out_path);
        //auto dur_ac = (std::chrono::steady_clock::now() - t_ac).count();
        //std::cout << "Time for adaptive compression: " << format_time(dur_ac) << std::endl;
        if (!std::filesystem::exists(out_path)) {
            std::cerr << "Adaptive compression did not produce an output!!! Path: " << out_path << std::endl;
            std::exit(1);
        }
    }
*/
