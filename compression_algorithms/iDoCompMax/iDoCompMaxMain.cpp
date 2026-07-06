//
// Created by mpelo on 8/6/24.
//

#include "include/iDoCompMax/iDoCompMaxMain.hpp"
#include "include/iDoCompMax/matching_rlz.hpp"
#include "include/iDoCompMax/iDoCompMaxPostprocessing.hpp"
#include "utils/PrettyPrint.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <memory>
#include "common/SteadyTimer.hpp"
#include <omp.h>

IDC_Encoding iDoCompMaxEncoding = HUFF;

int serialize_factors(const std::filesystem::path& output_file, size_t size_target, const char *fasta_line, size_t n_fasta_line,
                      const std::vector<struct match>& matches, const std::vector<struct substitution>& subs,
                      const std::vector<struct insertion> insertions);
int deserialize_factors(const std::filesystem::path& input_file, size_t& size_target, std::string& fasta_line,
                        std::vector<struct match>& matches, std::vector<struct substitution>& subs,
                                std::vector<struct insertion>& insertions);
void print_compression_stats(size_t n_matches, size_t n_insertions, size_t n_subs, size_t n_bytes);

std::size_t file_size_bytes(const std::filesystem::path& path) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
    file.open(path, std::ifstream::binary);
    file.seekg(0, file.end);
    std::size_t res = file.tellg();
    file.close();
    return res;
}

/**
 * Returns a pointer to a char array containing the FASTA sequence pointed to by path. If no error happens,
 * the returned pointer and fasta_line should be freed after use. The parameter n is set to the
 * size of the buffer without the terminating null character.
 * If any error happens, the returned pointer is nullptr, and all the pointers are freed by this call.
 * Also, an error line is printed to stderr detailing what the error was.
 * @param path
 * @param fasta_line
 * @return
 */
char *read_fasta(const std::filesystem::path& path, char **fasta_line, size_t *n, size_t *n_fasta_line) {
    size_t size = file_size_bytes(path);
    FILE *stream = fopen(path.c_str(), "r");
    if (!stream) {
        std::cerr << "Error while trying to open " << path << std::endl;
        return nullptr;
    }
    //Read the FASTA line if it is there
    *fasta_line = nullptr;
    //Peek to see if the fasta line is there
    int c;
    c = fgetc(stream);
    ungetc(c, stream);
    if (c == '>') {
        size_t alloc_len = 0;
        ssize_t n_read = 0;
        n_read = getline(fasta_line, &alloc_len, stream);
        if (n_read == -1) {
            std::cerr << "Error reading the file " << path << std::endl;
            free(*fasta_line);
            return nullptr;
        }
        *n_fasta_line = n_read; //We do not count the null byte because we will convert this to std::string later
        size -= n_read;
    } else {
        *fasta_line = static_cast<char*>(malloc(2));
        **fasta_line = '\n';
        *((*fasta_line)+1) = '\0';
        *n_fasta_line = 1;
    }
    char *ref_buf = static_cast<char*>(malloc(size + 1));
    std::size_t n_ref = fread(ref_buf, 1, size, stream);
    if (n_ref < size) {
        //Some error occurred
        std::cerr << "Error while reading reference sequence" << std::endl;
        if (*fasta_line != nullptr) {
            free(*fasta_line);
        }
        free(ref_buf);
        return nullptr;
    }
    fclose(stream);
    *n = size;
    ref_buf[size] = '\0';
    return ref_buf;
}

char *read_fasta(std::istream& in, char **fasta_line, size_t *n, size_t *n_fasta_line) {
    //Find size of the stream
    in.seekg(0, in.end);
    size_t size = in.tellg();
    in.seekg(0, in.beg);
    *fasta_line = nullptr;
    //Peek to see if the fasta line is there
    char c = in.peek();
    if (c == '>') {
        std::string fasta;
        std::getline(in, fasta);
        fasta.append("\n");
        //Copy string into raw array cuz that's how iDoComp works
        size_t fasta_line_len = fasta.size();
        *fasta_line = static_cast<char*>(malloc(fasta_line_len + 1));
        fasta.copy(*fasta_line, fasta_line_len);
        (*fasta_line)[fasta_line_len] = '\0';
        *n_fasta_line = fasta_line_len;
        size_t curr_pos = static_cast<size_t>(in.tellg());
        size = size - curr_pos;
    } else {
        *fasta_line = static_cast<char*>(malloc(2));
        (*fasta_line)[0] = '\n';
        (*fasta_line)[1] = '\0';
        *n_fasta_line = 1;
    }
    char *ref_buf = static_cast<char*>(malloc(size + 1));
    in.read(ref_buf, size);
    if (!in) {
        std::cerr << "Error while reading reference sequence at iDoCompMaxRef" << std::endl;
        if (*fasta_line != nullptr) {
            free(*fasta_line);
        }
        free(ref_buf);
        return nullptr;
    }
    *n = size;
    ref_buf[size] = '\0';
    return ref_buf;
}

/**
 * Reads a suffix array from a file as a sequence of ref_size ints, and returns the buffer of ints.
 * If an error occurred, it is printed to the standard error and a null pointer is returned.
 * @param path
 * @param ref_size
 * @return
 */
int *read_sa(const std::filesystem::path& path, size_t ref_size) {
    //Read SA
    if (!std::filesystem::exists(path)) {
        std::cerr << "SA not found at " << path << "!!!" << std::endl;
        return nullptr;
    }
    size_t sa_size = file_size_bytes(path);
    if (sa_size < sizeof(int)*ref_size) {
        std::cerr << "Bad SA (too small)" << std::endl;
    }
    int *sa_buf = static_cast<int*>(malloc(ref_size * sizeof(int) + 1));
    FILE *sa_stream = fopen(path.c_str(), "r");
    if (!sa_stream) {
        std::cerr << "Error opening SA file " << path << std::endl;
        free(sa_buf);
        return nullptr;
    }
    size_t n_read = fread(sa_buf, sizeof(int), ref_size, sa_stream);
    if (n_read < ref_size) {
        std::cerr << "Error reading SA " << path << std::endl;
        free(sa_buf);
        fclose(sa_stream);
        return nullptr;
    }
    fclose(sa_stream);
    return sa_buf;
}

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

void make_arithmetic_compression(const std::string& fasta_line, size_t size_target,
                                 const std::vector<struct match>& matches, const std::vector<struct substitution>& subs,
                                 const std::vector<struct insertion>& ins, std::ostream& out) {
    char *chars;
    size_t size_encoded;
    //make_delta_encoding(fasta_line, size_target, matches, subs, ins, &chars, size_encoded);
    multi_encode(fasta_line, size_target, matches, subs, ins, &chars, &size_encoded);
    //Write output
    out.write(chars, size_encoded);
    delete[] chars;
}

void make_arithmetic_decompression(std::istream& in_file, std::string& fasta_line, size_t& size_target,
                                   std::vector<struct match>& matchings, std::vector<struct substitution>& subs,
                                   std::vector<struct insertion>& insertions) {
    ProfilerSteady::profile_thread("decompress_target_" + std::to_string(omp_get_thread_num()), [&] {
        in_file.seekg(0, in_file.end);
        size_t length = in_file.tellg();
        in_file.seekg(0, in_file.beg);
        char *buf = new char[length];
        in_file.read(buf, length);
        //make_delta_decoding(buf, length, fasta_line, size_target, matchings, subs, insertions);
        multi_decode(buf, fasta_line, size_target, matchings, subs, insertions);
        delete[] buf;
    }, false);
}
iDoCompMaxRef::iDoCompMaxRef(const std::filesystem::path &ref_path, bool decomp) : RefDataStructure(ref_path) {
    //Read ref and ref_sa
    char *fasta_ref_line = nullptr;
    n_ref = 0;
    size_t n_fasta_line_ref = 0;
    ref = read_fasta(ref_path, &fasta_ref_line, &n_ref, &n_fasta_line_ref);
    if (!ref) {
        //read_fasta already printed an error message
        throw std::ios_base::failure("Failure to construct reference object for " + ref_path.string());
    }
    if (n_ref == 0) {
        throw std::ios_base::failure("Reference " + ref_path.string() + " empty!");
    }
    free(fasta_ref_line); //Not used
    if (!decomp) {
        ref_sa = read_sa(ref_path.string() + ".sa", n_ref);
        if (!ref_sa) {
            //read_sa already printed an error message
            throw std::ios_base::failure("Failure to read SA: " + ref_path.string() + ".sa");
        }
    } else {
        ref_sa = nullptr;
    }
}

iDoCompMaxRef::iDoCompMaxRef(std::istream& ref_path, bool decomp) : RefDataStructure(ref_path) {
    if (decomp == false)
        throw std::runtime_error("Can't compress without a SA!!!");
    //Read ref and ref_sa
    char *fasta_ref_line = nullptr;
    n_ref = 0;
    size_t n_fasta_line_ref = 0;
    ref = read_fasta(ref_path, &fasta_ref_line, &n_ref, &n_fasta_line_ref);
    if (!ref) {
        //read_fasta already printed an error message
        throw std::ios_base::failure("Failure to construct reference object for iDoCompMax reference");
    }
    if (n_ref == 0) {
        throw std::ios_base::failure("Reference for iDoCompMax empty!");
    }
    free(fasta_ref_line); //Not used
    ref_sa = nullptr;
}

void iDoCompMaxRef::compress_target(const std::filesystem::path &target_path, std::ostream& output_file) const {
    char *fasta_target_line = nullptr;
    size_t n_target = 0, n_fasta_line_target = 0;
    char *target_buf = read_fasta(target_path, &fasta_target_line, &n_target, &n_fasta_line_target);
    if (!target_buf) {
        throw std::runtime_error("At iDoCompMaxRef::compress_target: read_fasta failed");
    }
    if (n_target == 0) {
        throw std::runtime_error("Target empty!!!");
    }
    int ret;
    try {
        ret = compress_to_disk(target_buf, fasta_target_line, n_target,
                       n_fasta_line_target, output_file);
    } catch (std::exception& e) {
        std::cerr << "Exception when compressing with iDoCompMax: " << e.what() << std::endl;
        ret = -1;
    }
    if (ret != 0) {
        throw std::runtime_error("Error code " + std::to_string(ret) + " when compressing with iDoCompMax");
    }
    free(fasta_target_line);
    free(target_buf);
}

int iDoCompMaxRef::compress_to_disk(char *target, char *target_fasta_line, size_t n_target, size_t n_target_fasta_line,
                                    std::ostream& output_file) const {
    //First, we do the RLZ matching
    std::vector<struct match> matches = rlz_match(ref, ref_sa, target, n_ref, n_target);
    //Now, post-processing
    std::vector<struct substitution> subs;
    make_substitutions(ref, n_ref, matches, subs);
    //Now insertions
    std::vector<struct insertion> insertions;
    make_insertions(matches, insertions);
    //Align fasta line: if the size is not a multiple of 4, extend it
    std::string string_fasta_line = std::string(target_fasta_line, n_target_fasta_line);
    int rem = 4 - (string_fasta_line.size() % 4);
    if (rem != 4) {
        string_fasta_line.insert(string_fasta_line.size() - 1, rem, ' '); //Insert before newline
    }
    //We serialize the matches
    make_arithmetic_compression(string_fasta_line, n_target, matches, subs, insertions, output_file);
    //return serialize_factors(get_output_file, n_target, target_fasta_line, n_target_fasta_line,
    //                         matches_after_insertions, subs, insertions);
    //print_reconstruction(ref, target, n_ref, n_target, new_matches, subs);
    //Return OK
    return 0;
}

iDoCompMaxRef::~iDoCompMaxRef()  {
    if (ref != nullptr) {
        free(ref);
    }
    if (ref_sa != nullptr) {
        free(ref_sa);
    }
}

void iDoCompMaxRef::decompress_target(std::istream& target_input, std::ostream& output_file) const {
    //Read compressed target
    std::vector<struct match> matches;
    std::vector<struct substitution> subs;
    std::vector<struct insertion> ins;
    size_t size_target;
    std::string target_fasta_line;
    make_arithmetic_decompression(target_input,  target_fasta_line, size_target, matches, subs, ins);
    ProfilerSteady::profile_thread("decompress_target_" + std::to_string(omp_get_thread_num()), [&] {
        //Reconstruct
        char *target = new char[target_fasta_line.size() + 1 + size_target + ins.size() + 1];
        //Copy the fasta line
        memcpy(target, target_fasta_line.c_str(), target_fasta_line.size());
        target[target_fasta_line.size()] = '\n';
        size_t target_i = target_fasta_line.size() + 1;
        //Now, target_i is the index where the sequence starts
        //Reconstruction loop
        //"Premature optimization is the root of all evil" - This can be optimized with memcpy's, but the loop becomes harder
        //and I don't know if I'll have to modify it even further later - so the basic loop is here first
        //Also: "Nothing's more permanent than a temporary solution"
        size_t curr_i = 0, factor_i;
        auto it_factors = matches.begin();
        auto it_factors_end = matches.end();
        //Insertions are sorted by position already. We can use an iterator to check for the insertion position.
        auto it_ins = ins.begin();
        auto it_ins_end = ins.end();
        while (it_factors != it_factors_end) {
            struct match factor = *it_factors++;
            factor_i = 0;
            //copy factor and possible insertions
            while (factor_i < factor.len) {
                if (it_ins != it_ins_end && curr_i == it_ins->pos) {
                    //insertion position
                    target[target_i + curr_i] = it_ins->ch;
                    curr_i++;
                    it_ins++;
                } else {
                    target[target_i + curr_i] = ref[factor.pos + factor_i];
                    curr_i++;
                    factor_i++;
                }
            }
            //We copied the factor. Now, the mismatch
            while (it_ins != it_ins_end && curr_i == it_ins->pos) {
                target[target_i + curr_i] = it_ins->ch;
                curr_i++;
                it_ins++;
            }
            target[target_i + curr_i] = factor.mismatch;
            curr_i++;
        }
        //Maybe there are still insertions at the end?
        while (it_ins != it_ins_end && curr_i == it_ins->pos) {
            target[target_i + curr_i] = it_ins->ch;
            curr_i++;
            it_ins++;
        }
        //End of target
        target[target_i + curr_i] = '\0';
        //Apply substitutions
        for (struct substitution sub: subs) {
            target[target_i + sub.pos] = sub.ch;
        }
        //Write to file
        output_file.write(target, target_i + curr_i);
        //Free the target
        delete[] target;
    }, false);
}

void iDoCompMaxRef::parseRef(std::istream& in) {}

/**
 * PRINT METHODS
 */


void print_compression_stats(size_t n_matches, size_t n_insertions, size_t n_subs, size_t n_bytes) {
    std::cout << n_matches << ";" << n_insertions << ";" << n_subs << ";" << n_bytes << ";" << format_size(n_bytes) << "\n";
}

/**********************************************
 * LEGACY METHODS THAT MAY BE USEFUL LATER
 *
 *
 *
************************************************/
/*

int print_reconstruction(const char *ref, const char *target, size_t n_ref, size_t n_target,
                         const std::vector<struct match> matches, const std::vector<struct substitution> subs) {
    //Reconstruction!
    std::cout << "Factors: ";
    for (struct match factor : matches) {
        std::cout << "(" << factor.pos << ", " << factor.len << ", " << factor.mismatch << ")";
    }
    std::cout << std::endl;
    std::cout << "Reconstruction: ";
    std::string rec;
    rec.reserve(n_target);
    for (struct match factor : matches) {
        for (size_t i = 0; i < factor.len; i++) {
            rec += ref[factor.pos + i];
        }
        rec += factor.mismatch;
    }
    for (struct substitution sub : subs) {
        rec[sub.pos] = sub.ch;
    }
    std::cout << rec << std::endl;
    return 0;
}
*/
/**
 * We are going to do a very basic serialization for now
 * @param output_file
 * @param matches
 * @param subs
 * @return
 */
 /*
int serialize_factors(const std::filesystem::path& output_file, size_t size_target, const char *fasta_line, size_t n_fasta_line,
                      const std::vector<struct match>& matches, const std::vector<struct substitution>& subs,
                      const std::vector<struct insertion> insertions) {
    std::ofstream f(output_file, std::ios_base::trunc);
    if (!f) {
        std::cerr << "Error opening file " << get_output_file << std::endl;
        return -1;
    }
    if (fasta_line == nullptr) {
        fasta_line = "\n";
    }
    f << fasta_line;
    f << size_target << std::endl;
    f << matches.size() << std::endl;
    for (struct match factor : matches) {
        f << " " << factor.pos << " " << factor.len << " " << factor.mismatch;
    }
    f << std::endl;
    f << subs.size();
    for (struct substitution sub : subs) {
        f << " " << sub.pos << " " << sub.ch;
    }
    f << std::endl;
    f << insertions.size();
    for (struct insertion ins : insertions) {
        f << " " << ins.pos << " " << ins.ch;
    }
    f << std::endl;
    f.close();
    return 0;
}
*/
 /*
int deserialize_factors(const std::filesystem::path& input_file, size_t& size_target, std::string& fasta_line,
                        std::vector<struct match>& matches, std::vector<struct substitution>& subs,
                        std::vector<struct insertion>& insertions) {
    std::ifstream f(input_file, std::ios_base::in);
    if (!f) {
        std::cerr << "Error reading file " << input_file << std::endl;
        return -1;
    }
    std::getline(f, fasta_line);
    size_t matches_size, subs_size, insertions_size;
    f >> size_target;
    f >> matches_size;
    matches.reserve(matches_size);
    for (size_t i = 0; i < matches_size; i++) {
        size_t pos, len;
        char mismatch;
        f >> pos >> len >> mismatch;
        matches.emplace_back(pos, len, mismatch);
    }
    //Check that the size is correct (debug)
#ifndef NDEBUG
    size_t t = 0;
    for (struct match factor : matches) {
        t += factor.len + 1;
    }
    if (t > size_target) {
        std::cerr << "factors bigger than the target!!!" << std::endl;
        std::exit(-1);
    }
#endif
    f >> subs_size;
    subs.reserve(subs_size);
    for (size_t i = 0; i < subs_size; i++) {
        size_t pos;
        char ch;
        f >> pos >> ch;
        subs.emplace_back(pos, ch);
    }
    f >> insertions_size;
    insertions.reserve(insertions_size);
    for (size_t i = 0; i < insertions_size; i++) {
        size_t pos;
        char ch;
        f >> pos >> ch;
        insertions.emplace_back(pos, ch);
    }
    f.close();
    return 0;
}
*/