//
// Created by mpelo on 8/6/24.
//

#include "HRLZTests.h"

#include "test_idocomp_max.hpp"
#include "iDoCompMax/iDoCompMaxMain.hpp"
#include "utils_tests.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <HuffmanFile.hpp>

#include "../CppEngine/json.hpp"
#include "Compressors.hpp"

using json = nlohmann::json;

char *test_idocomp_parse_rlz(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    std::vector<struct match> matches = test_compress_idocompmax(ref, target);
    json ret;
    ret["return"] = json::array();
    for (struct match factor : matches) {
        ret["return"].push_back({{"__match__", true}, {"pos", factor.pos}, {"len", factor.len}, {"ch", factor.mismatch}});
    }
    return make_C_string(ret.dump());
}

char *test_idocompmax_parse_subs(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    std::vector<struct match> matches;
    std::vector<struct substitution> subs;
    int ret = test_idocompmax_parse_subs(ref, target, matches, subs);
    if (ret == -1) {
        return json_return_C("error");
    }
    json j;
    j["return"] = { {"matches", json::array()}, {"subs", json::array()} };
    for (struct match factor : matches) {
        j["return"]["matches"].push_back({{"__match__", true}, {"pos", factor.pos}, {"len", factor.len}, {"ch", factor.mismatch}});
    }
    for (struct substitution sub : subs) {
        j["return"]["subs"].push_back({{"__sub__", true}, {"pos", sub.pos}, {"ch", sub.ch}});
    }
    return make_C_string(j.dump());
}

char *test_idocompmax_insertions_decomp(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    int64_t ret = test_idocompmax_insertions_decomp(ref, target);
    return json_return_C(std::to_string(ret));
}

int make_strings_and_sa(const std::string& ref, const std::string& target,
                        std::filesystem::path& ref_path, std::filesystem::path& ref_sa_path,
                        std::filesystem::path& target_path, int **sa) {
    int st = serialize_two(ref, target, ref_path, target_path);
    if (st < 0) return -1;
    std::filesystem::path dir = ref_path.parent_path();
    //Make the SA
    ref_sa_path = std::filesystem::path(ref_path); //Copy constructor
    ref_sa_path += ".sa";
    try {
        //Call sa.run
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            execlp("sa_fasta.run", "sa_fasta.run", ref_path.c_str(), ref_sa_path.c_str(), (char *) 0);
            //If it reaches here, something went wrong
            std::cerr << "The SA for a test fasta couldn't be made" << std::endl;
            return -1;
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (status != 0) {
                std::cerr << "SA for a test fasta finished with error code " << status << std::endl;
                return -1;
            }
        }
    } catch (std::ios_base::failure& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    //SA done. Read it
    //The size of ref should be without the FASTA definition line
    size_t ref_size = ref.size();
    unsigned char c = ref[0];
    if (c == '>') {
        std::string::size_type pos_newline;
        pos_newline = ref.find('\n');
        ref_size -= (pos_newline + 1);
    }
    try {
        *sa = read_sa(ref_sa_path, ref_size);
        return 0;
    } catch (std::ios_base::failure& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

std::vector<struct match> test_compress_idocompmax(const std::string& ref, const std::string& target) {
    //Serialize and read everything
    std::filesystem::path ref_path, ref_sa_path, target_path;
    int *sa = nullptr;
    int ret = make_strings_and_sa(ref, target, ref_path, ref_sa_path, target_path, &sa);
    if (ret == -1)
    {
        std::cerr << "IO Error!!!" << std::endl;
        return std::vector<struct match>();
    }

    //Now, we can do the matches
    std::vector<struct match> matches = rlz_match(const_cast<char *>(ref.c_str()), sa,
                                                   const_cast<char *>(target.c_str()), ref.size(), target.size());
    free(sa);
    //Delete temp info
    return matches;
}

/**
 * Please, don't pass FASTA definition lines here
 */
int test_idocompmax_parse_subs(const std::string& ref, const std::string& target,
                               std::vector<struct match>& matches, std::vector<struct substitution>& subs) {
    //Serialize and read everything
    std::filesystem::path ref_path, ref_sa_path, target_path;
    int *sa = nullptr;
    int ret = make_strings_and_sa(ref, target, ref_path, ref_sa_path, target_path, &sa);
    if (ret == -1) { return -1; }
    //Matches
    std::vector<struct match> matches_rlz = rlz_match(const_cast<char *>(ref.c_str()), sa, const_cast<char *>(target.c_str()),
            ref.size(), target.size());
    //Post-processing
    subs = std::vector<struct substitution>();
    make_substitutions(ref.c_str(), ref.size(), matches_rlz, subs);
    matches = std::move(matches_rlz);
    free(sa);
    //delete matches_rlz;
    //matches and subs are out parameters
    return 0;
}

/**
 * Compares compression and decompression with insertions, and checks that the decompressed
 * output is the same as the target.
 * @param ref
 * @param target
 * @return The byte where target and decompression differ, or -1 if they are exactly equal (passed test)
 */
int test_idocompmax_insertions_decomp(const std::string& ref, const std::string& target) {
    //Serialize and read everything
    std::filesystem::path ref_path, ref_sa_path, target_path;
    int *sa = nullptr;
    int ret = make_strings_and_sa(ref, target, ref_path, ref_sa_path, target_path, &sa);
    if (ret == -1) { return 0; }//fail at the first byte
    std::filesystem::path temp = std::filesystem::temp_directory_path();
    std::filesystem::path target_filename = target_path.filename();
    std::filesystem::path comp_dir = temp / "comp_idocompmax";
    std::filesystem::path decomp_dir = temp / "output_idocompmax";
    if (!std::filesystem::exists(comp_dir)) {
        std::filesystem::create_directory(comp_dir);
    }
    if (!std::filesystem::exists(decomp_dir)) {
        std::filesystem::create_directory(decomp_dir);
    }
    //Compression
    try {
        iDoCompMaxRef idc(ref_path, false);
        std::ofstream out_tar(comp_dir / target_filename, idc.out_tar_mode);
        idc.compress_target(target_path, out_tar);
        out_tar.close();
        const Encoder& enc = HuffmanEncoder();
        enc.encode_executable(comp_dir / target_filename, comp_dir / target_filename); //Adds extension
        std::filesystem::path with_ext = comp_dir / (target_filename.string() + enc.extension);
        if (!std::filesystem::exists(with_ext)) {
            std::cerr << "Can't find file " << with_ext << "!!!";
            return 0;
        }
        std::filesystem::remove(comp_dir / target_filename);
        //decomp
        std::filesystem::path decoded = decomp_dir / (target_filename.string() + "_dec");
        enc.decode_executable(comp_dir / target_filename, decoded);
        std::ifstream in_tar(decomp_dir / decoded, idc.in_decomp_tar_mode);
        std::ofstream out_tar_dec(decomp_dir / target_filename, idc.out_decomp_tar_mode);
        idc.decompress_target(in_tar, out_tar_dec);
        in_tar.close();
        out_tar_dec.close();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    //Check
    std::stringstream t_stream(target);
    //Read the output
    std::string out;
    std::ifstream decomp(decomp_dir / target_filename, std::ios::binary);
    if (!decomp) {
        std::cerr << "No decompression! " << std::endl;
        return 0;
    }
    std::stringstream buffer;
    buffer << decomp.rdbuf();
    int64_t byte_differ = compare_fasta(buffer, t_stream);
    //Delete files
    std::filesystem::remove(ref_path);
    std::filesystem::remove(ref_sa_path);
    std::filesystem::remove(target_path);
    std::filesystem::remove(comp_dir / target_filename);
    std::filesystem::remove(decomp_dir / target_filename);
    return byte_differ;
}