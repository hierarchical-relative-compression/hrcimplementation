//
// Created by mpelo on 8/6/24.
//

#include <iostream>
#include "iDoCompMax/iDoCompMaxMain.hpp"
#include <string.h>
#include "HuffmanFile.hpp"
#include <fstream>

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: iDoCompMaxExec -c/-d reference target output_dir [target_original_filename] (if decompressing)" << std::endl;
        std::exit(1);
    }
    std::filesystem::path reference = argv[2];
    if (strcmp(argv[1], "-d") == 0) {
        std::filesystem::path target = argv[3];
        std::filesystem::path output_dir = argv[4];
        std::string target_original_filename;
        if (argc == 5) {
            target_original_filename = reference.filename().string() + "_" + target.filename().string();
        } else {
            target_original_filename = argv[5];
        };
        //Decomp
        //Decode first

        HuffmanEncoder enc;
        std::string target_decoded_filename;
        std::string target_filename = target.filename().string();
        std::string target_filename_no_ext;
        if (target_filename.ends_with(".huf")) {
            target_decoded_filename = target_filename.substr(0, target_filename.size() - 4);
            target_filename_no_ext = target_filename.substr(0, target_filename.size() - 4);
        } else {
            target_decoded_filename = target_filename + "_dec";
            target_filename_no_ext = target_filename;
        };
        //enc.decode_executable(target.parent_path() / target_filename_no_ext, output_dir / target_decoded_filename);
        //Decompress
        iDoCompMaxRef idc(reference, true);
        //std::ifstream in_tar(output_dir / target_decoded_filename, idc.in_tar_mode);
        std::ifstream in_tar(target, idc.in_tar_mode);
        if (!in_tar)
            //throw std::runtime_error("Can't open " + (output_dir /target_decoded_filename).string() + " for decompression");
            throw std::runtime_error("Can't open " + target.string() + " for decompression");
        std::ofstream out(output_dir / target_original_filename, idc.out_tar_mode);
        if (!out)
            throw std::runtime_error("Can't open " + (output_dir / target_original_filename).string() + " for decompression");
        idc.decompress_target(in_tar, out);
        in_tar.close();
        out.close();
        //std::filesystem::remove(output_dir / target_decoded_filename);
    } else {
        std::filesystem::path target = argv[3];
        std::filesystem::path output_dir = argv[4];
        iDoCompMaxRef idc(reference, false);
        std::filesystem::path compressed = output_dir / (reference.filename().string() + "_" + target.filename().string());
        std::ofstream out_compressed(compressed, idc.out_tar_mode);
        idc.compress_target(target, out_compressed);
        out_compressed.close();
        /*
        HuffmanEncoder enc;
        enc.encode_executable(compressed, compressed); //will add extension
        std::filesystem::remove(compressed);
         */
    }
}