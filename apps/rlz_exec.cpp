//
// Created by mpelo on 1/9/26.
//

#include <iostream>
#include <fstream>
#include <filesystem>
#include <ShuffEncoder.hpp>
#include <string>
#include <string.h>
#include "rlz/rlz.hpp"

/**
 * A stringstream that exposes its get and put areas for reading. Adapted from StackOverflow
 */
class BufferStringStream : public std::stringstream, public std::basic_stringbuf<char> {
public:
    BufferStringStream() {
        static_cast<std::basic_ios<char>&>(*this).rdbuf(this);
    }
    char *c_str_get() {
        *pptr() = '\0';
        return eback();
    }
    char *c_str_put() {
        *pptr() = '\0';
        return pbase();
    }
};

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: rlz_exec -c/-d reference target output_dir [target_original_filename (only if decompressing)]" << std::endl;
        std::exit(1);
    }
    std::filesystem::path reference = argv[2];
    std::filesystem::path target = argv[3];
    std::filesystem::path output_dir = argv[4];
    if (strcmp(argv[1], "-d") == 0) {
        std::string target_original_filename;
        if (argc == 5) {
            target_original_filename = target.filename().string().append("_dec");
        } else if (argc == 6) {
            target_original_filename = argv[5];
        } else {
            std::cerr << "Usage: hirgcmax_exec -c/-d reference target output_dir [target_original_filename (only if decompressing)]" << std::endl;
            std::exit(1);
        }
        //Decode
        std::string tar_filename = target.filename();
        if (tar_filename.ends_with(".shuff")) {
            tar_filename = tar_filename.substr(0, tar_filename.size() - 6);
        }
        ShuffEncoder shuff;
        shuff.decode_executable(target.parent_path() / tar_filename, target); //Handles extension
        RLZCompress rlzCompress(reference, true);
        //Decomp
        std::ifstream in_tar(target, rlzCompress.in_decomp_tar_mode);
        std::ofstream out_tar(output_dir / target_original_filename, rlzCompress.out_decomp_tar_mode);
        rlzCompress.decompress_target(in_tar, out_tar);
        in_tar.close();
        out_tar.close();
        std::filesystem::remove(target); //Remove middle file
    } else {
        RLZCompress rlzCompress(reference, false);
        std::string filename_tar { reference.filename().string().append("_").append(target.filename().string()) };
        BufferStringStream out_tar;
        rlzCompress.compress_target(target, out_tar);
        char *comp_buf = out_tar.c_str_get();
        size_t out_tar_size = out_tar.tellp();

        FILE *in_tar_comp = fmemopen(comp_buf, out_tar_size, "rb");
        FILE *out_encoded = fopen((output_dir / filename_tar).c_str(), "wb");
        ShuffEncoder shuff;
        shuff.encode_C_stream(in_tar_comp, out_encoded);
        fclose(out_encoded);
        fclose(in_tar_comp);
    }
}