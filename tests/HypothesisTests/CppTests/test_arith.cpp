//
// Created by mpelo on 8/10/24.
//

#include "HRLZTests.h"

#include "AdaptiveScalingCoder.hpp"
#include <iostream>
#include <fstream>
#include "utils_tests.hpp"
#include "test_arith.hpp"

int compress_decompress_arith(char *str, uint64_t n) {
    std::string s(str, n);
    return compress_decompress_arith(s);
}

int compress_decompress_arith(std::string& str) {

    AdaptiveScalingCoder coder(false);
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    int now_id = id++;
    //Test: write source string
    std::string string_decomp = "string_decomp_" + std::to_string(now_id);
    std::ofstream source(temp_dir / string_decomp);
    source.write(str.c_str(), str.size());
    source.close();
    std::string filename = "string " + std::to_string(now_id);
    try {
        std::string copy = str;
        coder.encode(copy, temp_dir / filename);
    } catch (std::exception& e) {
        std::cerr << e.what();
        std::filesystem::remove(temp_dir / string_decomp);
        return 0;
    }
    if (!std::filesystem::exists(temp_dir / filename)) {
        std::cerr << "No compression output found! " << std::endl;
        std::filesystem::remove(temp_dir / string_decomp);
        return 0;
    }
    //Compressed. Now, decompress
    std::filesystem::path output_dir = temp_dir / "idocompmax_decomp";
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directory(output_dir);
    }
    try {
        std::stringstream ss;
        std::filesystem::path out = output_dir / filename;
        coder.decode(temp_dir / filename, ss);
        ss.seekg(std::ios_base::beg);
        std::ofstream file(out, std::ios_base::binary);
        file << ss.rdbuf();
        file.close();
    } catch (std::exception& e) {
        std::cerr << e.what();
        std::filesystem::remove(temp_dir / string_decomp);
        return 0;
    }
    if (!std::filesystem::exists(output_dir / filename)) {
        std::cerr << "Decompression output not found!" << std::endl;
        std::filesystem::remove(temp_dir / string_decomp);
        return 0;
    }
    //Compare output
    std::string out;
    std::ifstream decomp(output_dir / filename, std::ios::binary);
    if (!decomp) {
        std::cerr << "Couldn't open decompression file for testing" << std::endl;
        std::filesystem::remove(temp_dir / string_decomp);
        return 0;
    }
    std::stringstream buffer;
    buffer << decomp.rdbuf();
    //Now, we have to check if the strings are the same
    std::stringstream original_str(str);
    int64_t byte_differ = compare_fasta(buffer, original_str);
    //Delete files
    std::filesystem::remove(temp_dir / filename);
    std::filesystem::remove(temp_dir / string_decomp);
    //std::filesystem::remove(output_dir / filename);
    return byte_differ;
}