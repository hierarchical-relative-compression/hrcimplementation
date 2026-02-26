//
// Created by mpelo on 8/6/24.
//

#include "include/iDoCompMax/matching_rlz.hpp"

size_t compare_sequences(char *ref, char *target, size_t n_ref, size_t n_target) {
    size_t i = 0;
    while (i < n_ref && i < n_target && ref[i] == target[i]) {
        i++;
    }
    return i;
}

std::vector<struct match> rlz_match(char *ref, int *ref_sa, char *target, size_t n_ref, size_t n_target) {
    size_t i = 0;
    std::vector<struct match> matches;
    matches.reserve(n_target); //This will avoid unnecessary relocations, will shrink to fit at the end
    //Basic RLZ matching with an unmatched character at the end
    while (i < n_target) {
        //We do a binary search on the SA to find where does T[i] fit
        //The upper end is exclusive
        size_t low = 0; size_t high = n_ref;
        while (low < high) {
            size_t sa_i = (low + high) / 2;
            size_t ref_suffix = ref_sa[sa_i];
            //We can only compare the sequence until n_target - i - 1 because the last character must be a mismatch
            size_t len = compare_sequences(ref + ref_suffix, target + i, n_ref - ref_suffix, n_target - i - 1);
            //Who's bigger, ref or target?
            //If we are at the end of ref, then ref must be smaller lexicographically (because target will always have
            //a mismatched character at the end of the match, and ref does not have a next character).
            if (ref_suffix + len == n_ref) {
                //target is larger
                low = sa_i + 1;
            } else if (ref[ref_suffix + len] > target[i + len]) {
                //Ref is larger, lower the range
                high = sa_i;
            } else {
                //Target is larger
                low = sa_i+1;
            }
        }
        //We end when low==high, and low is the smallest index for which the suffix SA[low] is larger than T[i].
        size_t pos = 0;
        size_t len = 0;
        if (low == 0) {
            //The longest matching suffix of ref is ref[ref_sa[0]]
            size_t ref_suffix = ref_sa[0];
            pos = ref_suffix;
            len = compare_sequences(ref + ref_suffix, target + i, n_ref - ref_suffix, n_target - i);
        } else if (low == n_ref) {
            size_t ref_suffix = ref_sa[n_ref - 1];
            pos = ref_suffix;
            len = compare_sequences(ref + ref_suffix, target + i, n_ref - ref_suffix, n_target - i);
        } else {
            //The longest matching suffix of ref is between low and low-1
            size_t ref_suffix = ref_sa[low];
            size_t comp = compare_sequences(ref + ref_suffix, target + i, n_ref - ref_suffix, n_target - i);
            size_t ref_suffix_1 = ref_sa[low-1];
            size_t comp_1 = compare_sequences(ref + ref_suffix_1, target + i, n_ref - ref_suffix_1, n_target - i);
            if (comp > comp_1) {
                pos = ref_suffix;
                len = comp;
            } else {
                pos = ref_suffix_1;
                len = comp_1;
            }
        }
        //Take care of the last character
        if (i + len == n_target) {
            len -= 1;
        }
        //Make match
        matches.emplace_back(pos, len, target[i+len]);
        //Advance pointer
        i += len + 1;
    }
    matches.shrink_to_fit();
    return matches;
}