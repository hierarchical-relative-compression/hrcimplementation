//
// Created by mpelo on 7/23/24.
//


#include "test_compress_sccg.hpp"
#include "../../../hrlz/include/hrlz/CompressorSCCG.hpp"
#include "utils_tests.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

/*
 * #include "HRLZTests.h"
int64_t test_compress_sccg(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    return test_compress_sccg(ref, target);
}
 */

int64_t test_compress_sccg(const std::string& ref_fasta, const std::string& target_fasta) {
    //Serialize ref and target
    static int id = 1;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::string string_id = std::to_string(id);
    std::string filename_ref = "ref" + string_id + ".fasta";
    std::string filename_target = "target" + string_id + ".fasta";
    id++;
    std::filesystem::path ref_path = temp_dir / filename_ref;
    std::filesystem::path target_path = temp_dir / filename_target;
    try {
        std::ofstream temp_ref(ref_path, std::ios::out | std::ios::trunc);
        std::ofstream temp_target(target_path, std::ios::out | std::ios::trunc);
        temp_ref.write(ref_fasta.c_str(), ref_fasta.size());
        temp_target.write(target_fasta.c_str(), target_fasta.size());
        temp_ref.close();
        temp_target.close();
    } catch (std::ios_base::failure& e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }
    //The files are in the disk. Compress
    //We are using the interface of hrlz because it already contains the code to handle the pipe.
    std::filesystem::path comp_dir = temp_dir / "comp_sccg";
    if (!std::filesystem::exists(comp_dir))
        std::filesystem::create_directory(comp_dir);
    //filesystem::output_dir = comp_dir;
    try {
        Compressors::CompressorSCCG *comp = new Compressors::CompressorSCCG();
        comp->output_dir = comp_dir;
        comp->compress_pair_keep(ref_path, target_path);
        std::string output_filename = filename_target + ".7z";
        std::filesystem::path compressed_path = comp_dir / output_filename;
        std::filesystem::path decomp_dir = temp_dir / "decomp_sccg";
        if (!std::filesystem::exists(decomp_dir))
            std::filesystem::create_directory(decomp_dir);
        //filesystem::output_dir = decomp_dir;
        comp->output_dir = decomp_dir;
        std::cout << "Compressing with SCCG" << std::endl;
        comp->decompress_pair(ref_path, compressed_path);
        std::cout << "Finished compressing!" << std::endl;
        //Read output and check
        std::filesystem::path decompressed_path = decomp_dir / filename_target;
        std::ifstream decomp(decompressed_path, std::ios::binary);
        std::stringstream buffer;
        buffer << decomp.rdbuf();
        //Check if the strings are the same
        std::stringstream target_stream(target_fasta);
        int64_t byte_differ = compare_fasta(buffer, target_stream);
        std::filesystem::remove(compressed_path);
        std::filesystem::remove(decompressed_path);
        delete comp;
        return byte_differ;
    } catch (std::exception& e) {
        //Time to remove all files
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        return 0;
    }
}