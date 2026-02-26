//
// Created by mpelo on 8/7/24.
//

#include <iostream>
#include <sstream>
#include "include/iDoCompMax/iDoCompMaxPostprocessing.hpp"
#include <cstring>
#include <cstdint>

#define SHORT_FACTOR_LEN 400
#define MIN_GAP 10
#define MIN_SUBS 25
#define VERY_SHORT_FACTOR 20

/**
 * Preprocess the sequence of matches to find small substitutions.
 * DOES NOT DELETE original_matchings.
 * @param original_matchings
 * @param new_substitutions
 * @param new_matchings
 */
void make_substitutions(const char *ref, size_t n_ref, std::vector<struct match>& original_matchings,
                       std::vector<struct substitution>& new_substitutions) {
    new_substitutions = std::vector<struct substitution>();
    //REMOVE: first the 1-substitutions
    struct match *original_matches = original_matchings.data();
    struct match *prev_match = original_matches;
    struct match *current_match = nullptr;
    size_t n_matches = original_matchings.size();
    size_t target_i = prev_match->len + 1; //target_i is at the start of the new match
    size_t new_matches_i = 1;
    for (size_t match_i = 1; match_i < n_matches; match_i++) {
        current_match = original_matches + match_i;
        if (current_match->pos == prev_match->pos + prev_match->len + 1) {
            //Add a substitution
            new_substitutions.emplace_back(target_i-1, prev_match->mismatch);
            //Modify old match
            prev_match->len = prev_match->len + current_match->len + 1;
            prev_match->mismatch = current_match->mismatch;
        } else if (current_match->len < SHORT_FACTOR_LEN &&
                   abs(current_match->pos - (prev_match->pos + prev_match->len)) > MIN_GAP &&
                   prev_match->pos + prev_match->len + current_match->len < n_ref) { //This last check is to make sure we don't overrun ref
            //Second type of substitution check
            bool do_substitution = current_match->len < VERY_SHORT_FACTOR;
            if (!do_substitution) {
                //Compute number of substitutions (without taking old_match->mismatch into account)
                size_t n_subs = 0;
                for (size_t i = 0; i < current_match->len; i++) {
                    if (ref[prev_match->pos + prev_match->len + 1 + i] != ref[current_match->pos + i]) {
                        n_subs++;
                    }
                }
                do_substitution = n_subs < MIN_SUBS;
            }
            if (do_substitution) {
                //Compute the substitutions
                //First, we add the one in old_match (we know it's a substitution)
                new_substitutions.emplace_back(target_i - 1, prev_match->mismatch);
                //Now, we add all the other subs
                size_t old_match_i = prev_match->pos + prev_match->len + 1;
                size_t current_match_i = current_match->pos;
                for (size_t i = 0; i < current_match->len; i++) {
                    if (ref[old_match_i + i] != ref[current_match_i + i]) {
                        new_substitutions.emplace_back(target_i + i, ref[current_match_i + i]);
                    }
                }
                //Modify old match
                prev_match->len = prev_match->len + 1 + current_match->len;
                prev_match->mismatch = current_match->mismatch;
                //If the last character turns out to not be a mismatch, it could be picked up by the next iteration
                //of substitutions
            } else {
                //No substitution between the matches
                prev_match++;
                *prev_match = *current_match;
                new_matches_i++;
            }
        } else {
            //No substitution between the matches
            prev_match++;
            *prev_match = *current_match;
            new_matches_i++;
        }
        //target_i advances by the processed match
        target_i += current_match->len + 1;
    }
    //Resize the vector of matches to its new length
    original_matchings.resize(new_matches_i);
}

void make_insertions(std::vector<struct match>& original_matchings,
                    std::vector<struct insertion>& new_insertions) {
    struct match *original_matches = original_matchings.data();
    struct match *old_match = original_matches;
    const struct match *current_match;
    size_t n_matches = original_matchings.size();
    size_t new_matches_i = 1;
    size_t target_i = old_match->len + 1; //target_i is at the start of current_match
    for (size_t match_i = 1; match_i < n_matches; match_i++) {
        current_match = original_matches + match_i;
        //Check if beginning of current_match and ending of the previous match are the same in ref
        if (old_match->pos + old_match->len == current_match->pos) {
            //There's an insertion at old_match->pos + old_match->len
            new_insertions.emplace_back(target_i - 1, old_match->mismatch);
            //Modify old match
            old_match->len = old_match->len + current_match->len;
            old_match->mismatch = current_match->mismatch;
        } else {
            //No insertion
            old_match++;
            *old_match = *current_match;
            new_matches_i++;
        }
        target_i += current_match->len + 1; //Note this
    }
    //Resize match vector
    original_matchings.resize(new_matches_i);
}

/**
 * The user will have to DELETE the array of ints after use.
 * Note: FASTA line MUST HAVE a newline character
 * Format:
 * ints: [fasta_line\n] size_target size_m size_s size_i [size_m match.pos] [size_m match.len] [size_s subs.pos] [size_i ins.pos] [mismatches] [signs] [subs.ch] [ins.ch] \0
 * everything above are 4 byte integers
 */
void make_delta_encoding(const std::string& fasta_line, size_t size_target,
                         const std::vector<struct match>& matchings, const std::vector<struct substitution>& subs,
                                 const std::vector<struct insertion>& ins, char **ints_ptr, size_t& size_ints) {
    size_t size_m = matchings.size();
    size_t size_s = subs.size();
    size_t size_i = ins.size();
    size_t n_ints = 4 + size_m * 2 + size_s + size_i;
    size_ints = fasta_line.size() + n_ints * sizeof(int) + 2*size_m + size_s + size_i + 1; //See format
    *ints_ptr = new char[size_ints];
    char *ints = *ints_ptr;
    //Write fasta line
    fasta_line.copy(ints, fasta_line.size());
    ints += fasta_line.size();
    //pointer to different int arrays
    int *match_ptr = reinterpret_cast<int*>(ints);
    int *len_ptr = reinterpret_cast<int*>(ints) + 4 + size_m;
    char *mismatch_ptr = ints + n_ints*sizeof(int);
    char *signs_ptr = mismatch_ptr + size_m;
    //Check if sizes fit in int
    if (size_target > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
    if (size_m > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
    if (size_s > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
    if (size_i > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
    //Write sizes
    *match_ptr++ = static_cast<int>(size_target);
    *match_ptr++ = static_cast<int>(size_m);
    *match_ptr++ = static_cast<int>(size_s);
    *match_ptr++ = static_cast<int>(size_i);
    //Write matches. Write pos as the difference to the previous pos
    int prev_pos = 0, pos;
    for (struct match factor : matchings) {
        pos = factor.pos - prev_pos;
        *signs_ptr++ = static_cast<char>(pos < 0 ? -1 : 1); //Note how this is only one byte
        prev_pos = factor.pos + factor.len;                 //Delta encoding
        pos = abs(pos);
        if (pos > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
        if (factor.len > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
        *match_ptr++ = static_cast<int>(pos);
        *len_ptr++ = static_cast<int>(factor.len);
        *mismatch_ptr++ = factor.mismatch;
    }
    //Write substitutions
    int *sub_pos = len_ptr; //The end of the previous part
    char *char_pos = signs_ptr;
    prev_pos = 0;
    for (struct substitution sub : subs) {
        pos = sub.pos - prev_pos;
        prev_pos = sub.pos;                 //Here the delta encoding does not consider any length
        if (pos > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
        *sub_pos++ = static_cast<int>(pos);
        *char_pos++ = sub.ch;
    }
    //Write insertions
    int *ins_pos = sub_pos; //for clarity
    prev_pos = 0;
    for (struct insertion i : ins) {
        pos = i.pos - prev_pos;
        prev_pos = i.pos;
        if (pos > INT32_MAX) { std::cerr << "Precision issue!!!" << std::endl; }
        *ins_pos++ = static_cast<int>(pos);
        *char_pos++ = i.ch;
    }
    *char_pos = '\0';
}

/**
 * Format:
 * ints: FASTA line\n size_target size_m size_s size_i [size_m match.pos] [size_m match.len] [size_s subs.pos] [size_i ins.pos] [mismatches] [signs] [subs.ch] [ins.ch]
 */
void make_delta_decoding(char *buf, size_t n, std::string& fasta_line, size_t& target_size, std::vector<struct match>& matchings,
        std::vector<struct substitution>& subs, std::vector<struct insertion>& ins) {
    (void)n;
    int size_m, size_s, size_i;
    char *read = buf;
    //There's always a FASTA line, though it may be empty
    char *newline_ptr = strchr(read, '\n');
    int fasta_line_len = newline_ptr - read;
    fasta_line = std::string(buf, fasta_line_len);
    int *matches_ptr = reinterpret_cast<int*>(newline_ptr + 1); //Skip the newline
    //Decode sizes
    target_size = *matches_ptr++;
    size_m = *matches_ptr++;
    size_s = *matches_ptr++;
    size_i = *matches_ptr++;
    matchings = std::vector<struct match>();
    matchings.reserve(size_m);
    subs = std::vector<struct substitution>();
    subs.reserve(size_s);
    ins = std::vector<struct insertion>();
    ins.reserve(size_i);
    //Decode the pos and len of matchings with delta compression. We also need the signs.
    //We need both the length and the position to decode the delta encoding
    char *signs = newline_ptr + 1 + sizeof(int)*(4 + size_m * 2 + size_s + size_i) + size_m;
    int prev_pos = 0, pos, len;
    int *len_ptr = matches_ptr + size_m;
    for (int i = 0; i < size_m; i++) {
        int8_t s = *signs++;
        pos = *matches_ptr++;
        len = *len_ptr++;
        pos = s * pos + prev_pos;
        prev_pos = pos + len;
        matchings.emplace_back(pos, len, 0);
    }
    int *subs_ptr = len_ptr; //for clarity
    //Decode the pos of subs and ins
    prev_pos = 0;
    for (int i = 0; i < size_s; i++) {
        pos = *subs_ptr++ + prev_pos;
        prev_pos = pos;
        subs.emplace_back(pos, 0);
    }
    prev_pos = 0;
    int *ins_ptr = subs_ptr;
    for (int i = 0; i < size_i; i++) {
        pos = *ins_ptr++ + prev_pos;
        prev_pos = pos;
        ins.emplace_back(pos, 0);
    }
    //Decode mismatches
    char *chars = reinterpret_cast<char*>(ins_ptr);
    for (int i = 0; i < size_m; i++) {
        matchings[i].mismatch = *chars++;
    }
    //We've decoded the signs already
    chars += size_m;
    //Decode the chars of subs and ins
    for (int i = 0; i < size_s; i++) {
        subs[i].ch = *chars++;
    }
    for (int i = 0; i < size_i; i++) {
        ins[i].ch = *chars++;
    }
}

/**
 * FASTA line MUST contain a newline character
 * @param fasta_line
 * @param size_target
 * @param matchings
 * @param subs
 * @param ins
 * @param encoded
 * @param size
 */
void multi_encode(const std::string& fasta_line, size_t size_target, const std::vector<struct match>& matchings,
        const std::vector<struct substitution>& subs, const std::vector<struct insertion>& ins, char **encoded, size_t *size) {
    size_t size_m = matchings.size();
    size_t size_s = subs.size();
    size_t size_i = ins.size();
    size_t fasta_size = fasta_line.size();
    size_t n_ints = 2 * size_m + size_s + size_i;
    size_t n_chars = size_m + size_s + size_i;
    *size = fasta_size + 16 + n_ints * 4 + n_chars + size_m;
    char *alloc = new char[*size]; //The last size_m are the signs
    fasta_line.copy(alloc, fasta_size);
    int *four_ints = reinterpret_cast<int*>(alloc + fasta_size);
    four_ints[0] = size_target;
    four_ints[1] = size_m;
    four_ints[2] = size_s;
    four_ints[3] = size_i;
    char *byte0 = alloc + fasta_size + 16;
    char *byte1 = byte0 + n_ints;
    char *byte2 = byte1 + n_ints;
    char *byte3 = byte2 + n_ints;
    char *len_byte_0 = byte0 + size_m + size_s + size_i;
    char *len_byte_1 = len_byte_0 + n_ints;
    char *len_byte_2 = len_byte_1 + n_ints;
    char *len_byte_3 = len_byte_2 + n_ints;
    char *chars = byte3 + n_ints;
    char *signs = chars + n_chars;
    //Write matches. Write pos as the difference to the previous pos
    int prev_pos = 0, pos, i = 0;
    char *pos_ptr = reinterpret_cast<char*>(&pos);
    int len;
    const char *len_ptr = reinterpret_cast<char*>(&len);
    for (const struct match& factor : matchings) {
        len = factor.len;
        pos = factor.pos - prev_pos;
        signs[i] = static_cast<char>(pos < 0 ? -1 : 1); //Note how this is only one byte
        prev_pos = factor.pos + len;                 //Delta encoding
        pos = abs(pos);
        //Write each byte of "pos" and "len"
        byte0[i] = pos_ptr[0];
        byte1[i] = pos_ptr[1];
        byte2[i] = pos_ptr[2];
        byte3[i] = pos_ptr[3];
        len_byte_0[i] = len_ptr[0];
        len_byte_1[i] = len_ptr[1];
        len_byte_2[i] = len_ptr[2];
        len_byte_3[i] = len_ptr[3];
        chars[i] = factor.mismatch;
        i++;
    }
    //Write substitutions
    byte0 += size_m;
    byte1 += size_m;
    byte2 += size_m;
    byte3 += size_m;
    chars += size_m;
    prev_pos = 0;
    i = 0;
    for (const struct substitution& sub : subs) {
        pos = sub.pos - prev_pos;
        prev_pos = sub.pos;                 //Here the delta encoding does not consider any length
        byte0[i] = pos_ptr[0];
        byte1[i] = pos_ptr[1];
        byte2[i] = pos_ptr[2];
        byte3[i] = pos_ptr[3];
        chars[i] = sub.ch;
        i++;
    }
    //Write insertions
    byte0 += size_s;
    byte1 += size_s;
    byte2 += size_s;
    byte3 += size_s;
    chars += size_s;
    prev_pos = 0;
    i = 0;
    for (const struct insertion& in : ins) {
        pos = in.pos - prev_pos;
        prev_pos = in.pos;
        byte0[i] = pos_ptr[0];
        byte1[i] = pos_ptr[1];
        byte2[i] = pos_ptr[2];
        byte3[i] = pos_ptr[3];
        chars[i] = in.ch;
        i++;
    }
    *encoded = alloc;
}

void multi_decode(char *buf, std::string& fasta_line, size_t& target_size, std::vector<struct match>& matchings,
        std::vector<struct substitution>& subs, std::vector<struct insertion>& ins) {
    int size_m, size_s, size_i;
    char *newline_ptr = strchr(buf, '\n');
    fasta_line = std::string(buf, newline_ptr - buf);
    int *ints = reinterpret_cast<int*>(newline_ptr + 1); //Skip the newline
    target_size = ints[0];
    size_m = ints[1];
    size_s = ints[2];
    size_i = ints[3];
    matchings = std::vector<struct match>();
    matchings.reserve(size_m);
    subs = std::vector<struct substitution>();
    subs.reserve(size_s);
    ins = std::vector<struct insertion>();
    ins.reserve(size_i);
    //Decode the pos and len of matchings with delta compression. We also need the signs.
    //We need both the length and the position to decode the delta encoding
    size_t n_ints = 2 * size_m + size_s + size_i;
    size_t n_chars = size_m + size_s + size_i;
    char *byte0 = reinterpret_cast<char*>(ints + 4);
    char *byte1 = byte0 + n_ints;
    char *byte2 = byte1 + n_ints;
    char *byte3 = byte2 + n_ints;
    char *len_byte0 = byte0 + size_m + size_s + size_i;
    char *len_byte1 = len_byte0 + n_ints;
    char *len_byte2 = len_byte1 + n_ints;
    char *len_byte3 = len_byte2 + n_ints;
    char *chars = byte3 + n_ints;
    char *signs = chars + n_chars;

    int prev_pos = 0, pos, len, i;
    char *pos_ptr = reinterpret_cast<char*>(&pos);
    char *len_ptr = reinterpret_cast<char*>(&len);
    for (i = 0; i < size_m; i++) {
        int8_t s = signs[i];
        pos_ptr[0] = byte0[i];
        pos_ptr[1] = byte1[i];
        pos_ptr[2] = byte2[i];
        pos_ptr[3] = byte3[i];
        len_ptr[0] = len_byte0[i];
        len_ptr[1] = len_byte1[i];
        len_ptr[2] = len_byte2[i];
        len_ptr[3] = len_byte3[i];
        pos = s * pos + prev_pos;
        prev_pos = pos + len;
        matchings.emplace_back(pos, len, chars[i]);
    }
    //Subs
    byte0 += size_m;
    byte1 += size_m;
    byte2 += size_m;
    byte3 += size_m;
    chars += size_m;
    //Decode the pos of subs and ins
    prev_pos = 0;
    for (i = 0; i < size_s; i++) {
        pos_ptr[0] = byte0[i];
        pos_ptr[1] = byte1[i];
        pos_ptr[2] = byte2[i];
        pos_ptr[3] = byte3[i];
        pos += prev_pos;
        prev_pos = pos;
        subs.emplace_back(pos, chars[i]);
    }
    //Ins
    byte0 += size_s;
    byte1 += size_s;
    byte2 += size_s;
    byte3 += size_s;
    chars += size_s;
    prev_pos = 0;
    for (i = 0; i < size_i; i++) {
        pos_ptr[0] = byte0[i];
        pos_ptr[1] = byte1[i];
        pos_ptr[2] = byte2[i];
        pos_ptr[3] = byte3[i];
        pos += prev_pos;
        prev_pos = pos;
        ins.emplace_back(pos, chars[i]);
    }
}





