//
// Created by mpelo on 7/22/24.
// To check if HiRGCMax is compressing and decompressing properly.
//

#include "HRLZTests.h"

#include "test_compress_hirgc_max.hpp"
#include "hirgc/hirgc_max.hpp"
#include "utils_tests.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <HuffmanFile.hpp>

#include "hrlz/Compressors.hpp"
#include "hrlz/CompressorHiRGCMax.hpp"

int64_t test_compress_hirgcmax(char *ref_fasta, uint64_t n_ref_fasta, char *target_fasta, uint64_t n_target_fasta, uint64_t random) {
    std::string ref(ref_fasta, n_ref_fasta);
    std::string target(target_fasta, n_target_fasta);
    return test_compress_hirgcmax(ref, target, random);
}

int64_t test_compress_hirgcmax(const std::string& ref_fasta, const std::string& target_fasta, uint64_t random) {
    //First, serialize the reference and the target.
    std::filesystem::path ref_path, target_path;
    serialize_two(ref_fasta, target_fasta, ref_path, target_path);
    std::filesystem::path temp_dir = ref_path.parent_path();
    //Files are in disk. Compress and decompress.
    try {
        hirgc_mrmcmax::HiRGCRef ref_obj(ref_path, random);
        std::string filename = hirgc_mrmcmax::get_filename(ref_path, target_path);
        std::filesystem::path compressed_path = temp_dir / filename;
        std::ofstream out_compressed(compressed_path, ref_obj.out_tar_mode);
        ref_obj.compress_target(target_path, out_compressed);
        out_compressed.close();
        //Encode
        const Encoder& enc = HuffmanEncoder();
        enc.encode_executable(compressed_path, compressed_path); //Will add extension
        std::filesystem::path path_with_extension = temp_dir / (filename + enc.extension);
        if (!std::filesystem::exists(path_with_extension)) {
            std::cerr << "Compressed file does not exist!" << std::endl;
            return 0;
        }
        std::filesystem::remove(compressed_path); //Remove file without extension

        std::string subdir = "decomp_hirgc_max";
        //We want to make a subdirectory for the decompressed files
        if (!std::filesystem::exists(temp_dir / subdir)) {
            std::filesystem::create_directory(temp_dir / subdir);
        }
        //Decode
        enc.decode_executable(compressed_path, compressed_path);
        std::ifstream comp_tar_in(compressed_path, ref_obj.in_decomp_tar_mode);
        std::ofstream out_tar(temp_dir / subdir / target_path.filename(), ref_obj.out_decomp_tar_mode);
        ref_obj.decompress_target(comp_tar_in, out_tar);
        comp_tar_in.close();
        out_tar.close();
        std::filesystem::path decompressed_target = temp_dir / subdir / target_path.filename();
        if (!std::filesystem::exists(decompressed_target)) {
            std::cerr << "Decompressed file does not exist!" << std::endl;
            return 0;
        }
        //Read the target
        std::ifstream decomp(decompressed_target, std::ios::binary);
        std::stringstream buffer;
        buffer << decomp.rdbuf();
        //Check if the strings are the same
        std::stringstream target_stream(target_fasta);
        int64_t byte_differ = compare_fasta(buffer, target_stream);
        //Time to remove all files
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        std::filesystem::remove(compressed_path);
        std::filesystem::remove(decompressed_target);
        return byte_differ;
    } catch (std::ios_base::failure& e) {
        std::cerr << "IOError while compressing with HiRGC" << std::endl;
        std::cerr << e.what() << std::endl;
        std::filesystem::remove(ref_path);
        std::filesystem::remove(target_path);
        return 0;
    }
}